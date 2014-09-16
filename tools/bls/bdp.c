// The Blast ! debugger

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>

#include "bls.h"

////// Globals

int genfd;
u8 inp[40];
int inpl;
int bdpdump = 0;

void senddata_ip(const u8 *data, int size);
void senddata_serial(const u8 *data, int size);
void (*senddata)(const u8 *data, int size);


// Return total packet length based on header value
int packetlen(u8 header)
{
  if((header & 0xC0) && (header & CMD_WRITE))
  {
    int l = header & CMD_LEN;
    if(!l)
    {
      return 36;
    }
    return l + 4;
  }
  return 4;
}



// TODO : make this compatible with non-blocking mode
int readdata()
{
  inpl = 0;
  if(bdpdump) printf("<");
  do
  {
    // Read the header byte, then the whole remaining packet
    ssize_t r = read(genfd, &inp[inpl], inpl ? packetlen(inp[0]) - inpl : 1);
    if(r == 0)
    {
      fprintf(stderr, "Could not read from genesis : connection closed.\n");
      exit(2);
    }
    if(r == -1)
    {
      if(errno == EAGAIN)
      {
        return 0;
      }
      if(errno != EINTR)
      {
        fprintf(stderr, "Could not read from genesis : %s\n", strerror(errno));
        exit(2);
      }
    }
    else
    {
      int b;
      if(bdpdump) for(b = 0; b < r; ++b)
      {
        printf("%02X", inp[inpl+b]);
      }
      inpl += r;
    }
  } while(inpl < packetlen(inp[0]));

  if(bdpdump) printf("\n");

  return 1;
}

void senddata_ip(const u8 *data, int size)
{
  if(bdpdump) printf(">");
  while(size)
  {    
    ssize_t w = write(genfd, data, size);
    if(w == 0)
    {
      fprintf(stderr, "Could not send to genesis : connection closed.\n");
      exit(2);
    }
    if(w == -1)
    {
      if(errno != EINTR)
      {
        fprintf(stderr, "Could not send to genesis : %s\n", strerror(errno));
        exit(2);
      }
    }
    else
    {
      int b;
      if(bdpdump) for(b = 0; b < w; ++b)
      {
        printf("%02X", data[b]);
      }
      data += w;
      size -= w;
    }
  }
  if(bdpdump) printf("\n");
}

void senddata_serial(const u8 *data, int size)
{
#define SERIAL_BUFFER 1
  if(bdpdump) printf(">");
  while(size)
  {
    // Empty buffer before sending data
    while(tcdrain(genfd) == -1 && errno == EINTR);

    // Avoid buffer overflow by limiting write size to SERIAL_BUFFER bytes
    ssize_t w = write(genfd, data, size < SERIAL_BUFFER ? size : SERIAL_BUFFER);
    if(w == 0)
    {
      fprintf(stderr, "Could not send to genesis : connection closed.\n");
      exit(2);
    }
    if(w == -1)
    {
      if(errno != EINTR)
      {
        fprintf(stderr, "Could not send to genesis : %s\n", strerror(errno));
        exit(2);
      }
    }
    else
    {
//      usleep(200 + SERIAL_BUFFER * w * 1000000 / (115200/8));
      int b;
      if(bdpdump) for(b = 0; b < w; ++b)
      {
        printf("%02X", data[b]);
      }
      data += w;
      size -= w;
    }
  }
  if(bdpdump) printf("\n");
}

void sendcmd(u8 header, u32 address)
{
  u8 packet[4];
  setint(address, packet, 4);
  packet[0] = header;
  senddata(packet, 4);
}

void sendbyte(u32 value)
{
  u8 d = value;
  senddata(&d, 1);
}

void sendword(u32 value)
{
  u8 d[2];
  setint(value, d, 2);
  senddata(d, 2);
}

void sendlong(u32 value)
{
  u8 d[4];
  setint(value, d, 4);
  senddata(d, 4);
}

#define SCD_1M 0x00040000
#define SCD_2M_SUB 0x00020000
static u32 substatus;
static int reqcnt = 0; // Counter used to support recursive subreq
void subreq()
{
  if(!reqcnt)
  {
    substatus = readbyte(0xA12001);
    substatus |= readword(0xA12002) << 16;
    writeword(0xA12000, 0x0003);
  }
  ++reqcnt;
}

void subsetbank(int bank)
{
  if(substatus & SCD_1M)
  {
    writeword(0xA12002, bank << 6);
  }
  else
  {
    // 2M mode
    if(substatus & SCD_2M_SUB)
    {
      writeword(0xA12002, (bank << 6) | 0x0002);
    }
    else
    {
      writeword(0xA12002, bank << 6);
    }
  }
}

void subrelease()
{
  if(!--reqcnt)
  {
    if(!(substatus & 0x0002))
      writebyte(0xA12001, 0x01); // Release busreq if bus was not taken.

    writeword(0xA12002, substatus >> 16);
  }
}

void subreset()
{
  if(reqcnt)
  {
    writeword(0xA12002, substatus >> 16);
    reqcnt = 0;
  }
  usleep(20000);
  writebyte(0xA12001, 0x01); // Release reset high
  usleep(80000);
  writebyte(0xA12001, 0x00); // Bring reset low
  usleep(80000);
  writebyte(0xA12001, 0x01); // Release reset high
}

void readburst(u8 *target, u32 address, int length)
{
  sendcmd(CMD_READ | CMD_BYTE | (CMD_LEN & length), address);
  readdata();
  if(inp[0] != (CMD_WRITE | CMD_BYTE | (CMD_LEN & length)))
  {
    printf("Genesis communication error. Received %02X instead of %02X.\n", inp[0], (CMD_WRITE | CMD_BYTE | (CMD_LEN & length) ));
    exit(2);
  }
  memcpy(target, &inp[4], length);
}

u32 readlong(u32 address)
{
  sendcmd(CMD_READ | CMD_LONG | 4, address);
  readdata();
  if(inp[0] != (CMD_WRITE | CMD_LONG | 4))
  {
    printf("Genesis communication error. Received %02X instead of %02X.\n", inp[0], (CMD_WRITE | CMD_LONG | 4));
    exit(2);
  }
  return getint(&inp[4], 4);
}

u32 readword(u32 address)
{
  sendcmd(CMD_READ | CMD_WORD | 2, address);
  readdata();
  if(inp[0] != (CMD_WRITE | CMD_WORD | 2))
  {
    printf("Genesis communication error. Received %02X instead of %02X.\n", inp[0], (CMD_WRITE | CMD_WORD | 2));
    exit(2);
  }
  return getint(&inp[4], 2);
}

u32 readbyte(u32 address)
{
  sendcmd(CMD_READ | CMD_BYTE | 1, address);
  readdata();
  if(inp[0] != (CMD_WRITE | CMD_BYTE | 1))
  {
    printf("Genesis communication error. Received %02X instead of %02X.\n", inp[0], (CMD_WRITE | CMD_BYTE | 1));
    exit(2);
  }
  return getint(&inp[4], 1);
}

// Reads memory as fast as possible
void readmem(u8 *target, u32 address, int length)
{
  while(length > 32)
  {
    readburst(target, address, 32);
    target += 32;
    address += 32;
    length -= 32;
  }
  readburst(target, address, length);
}

void sendburst(u32 address, const u8 *source, int length)
{
  sendcmd(CMD_WRITE | CMD_BYTE | (CMD_LEN & length), address);
  senddata(source, length);
}

// Send data into RAM as fast as possible
void sendmem(u32 address, const u8 *source, int length)
{
  while(length > 32)
  {
    sendburst(address, source, 32);
    source += 32;
    address += 32;
    length -= 32;
  }
  sendburst(address, source, length);
}

// Send data into RAM and verify data integrity
void sendmem_verify(u32 address, const u8 *source, int length)
{
  u8 verify[32];
  while(length > 32)
  {
sendmem_verify_retry:
    sendburst(address, source, 32);
    readburst(verify, address, 32);
    if(memcmp(source, verify, 32) != 0)
    {
      printf("Corrupt data at %06X. Retrying.\n", address);
      goto sendmem_verify_retry;
    }
    source += 32;
    address += 32;
    length -= 32;
  }

sendmem_verify_retry_last:
  sendburst(address, source, length);
  readburst(verify, address, length);
  if(memcmp(source, verify, length) != 0)
  {
    printf("Corrupt data at %06X. Retrying.\n", address);
    goto sendmem_verify_retry_last;
  }
}

void writelong(u32 address, u32 l)
{
  sendcmd(CMD_WRITE | CMD_LONG | 4, address);
  u8 d[4];
  setint(l, d, 4);
  senddata(d, 4);
}

void writeword(u32 address, u32 w)
{
  sendcmd(CMD_WRITE | CMD_WORD | 2, address);
  u8 d[2];
  setint(w, d, 2);
  senddata(d, 2);
}

void writebyte(u32 address, u32 b)
{
  sendcmd(CMD_WRITE | CMD_BYTE | 1, address);
  u8 d = b;
  senddata(&d, 1);
}

// Send data into sub PRAM
void subsendmem(u32 address, const u8 *source, int length)
{
  subreq();

  u32 lastaddr = address + length - 1;
  int firstbank = address / 0x20000;
  int lastbank = lastaddr / 0x20000;

  subsetbank(firstbank);
  if(lastbank == firstbank)
  {
    sendmem((address & 0x1FFFF) + 0x20000, source, length);
  }
  else
  {
    // Send first bank
    sendmem((address & 0x1FFFF) + 0x20000, source, 0x20000 - (address & 0x1FFFF));
    source += 0x20000 - (address & 0x1FFFF);

    // Send intermediate banks
    while(++firstbank < lastbank)
    {
      subsetbank(firstbank);
      sendmem(0x20000, source, 0x20000);
      source += 0x20000;
    }

    // Send last bank
    subsetbank(lastbank);
    sendmem(0x20000, source, (lastaddr & 0x1FFFF) + 1);
  }

  subrelease();
}

// Send data into sub PRAM with data integrity check.
void subsendmem_verify(u32 address, const u8 *source, int length)
{
  subreq();

  u32 lastaddr = address + length - 1;
  int firstbank = address / 0x20000;
  int lastbank = lastaddr / 0x20000;

  subsetbank(firstbank);
  if(lastbank == firstbank)
  {
    sendmem_verify((address & 0x1FFFF) + 0x20000, source, length);
  }
  else
  {
    // Send first bank
    sendmem_verify((address & 0x1FFFF) + 0x20000, source, 0x20000 - (address & 0x1FFFF));
    source += 0x20000 - (address & 0x1FFFF);

    // Send intermediate banks
    while(++firstbank < lastbank)
    {
      subsetbank(firstbank);
      sendmem_verify(0x20000, source, 0x20000);
      source += 0x20000;
    }

    // Send last bank
    subsetbank(lastbank);
    sendmem_verify(0x20000, source, (lastaddr & 0x1FFFF) + 1);
  }

  subrelease();
}

// Read data from sub PRAM
void subreadmem(u8 *target, u32 address, int length)
{
  subreq();

  u32 lastaddr = address + length - 1;
  int firstbank = address / 0x20000;
  int lastbank = lastaddr / 0x20000;

  subsetbank(firstbank);
  if(lastbank == firstbank)
  {
    readmem(target, (address & 0x1FFFF) + 0x20000, length);
  }
  else
  {
    // Send first bank
    readmem(target, (address & 0x1FFFF) + 0x20000, 0x20000 - (address & 0x1FFFF));
    target += 0x20000 - (address & 0x1FFFF);

    // Send intermediate banks
    while(++firstbank < lastbank)
    {
      subsetbank(firstbank);
      readmem(target, 0x20000, 0x20000);
      target += 0x20000;
    }

    // Send last bank
    subsetbank(lastbank);
    readmem(target, 0x20000, (lastaddr & 0x1FFFF) + 1);
  }

  subrelease();
}

void vdpsetreg(int reg, u8 value)
{
  writeword(VDPCTRL, 0x8000 | ((u32)reg << 8) | value);
}

void vramsendmem(u32 address, const u8 *source, int length)
{
  // Extremely slow. Should use VRAM buffering
  vdpsetreg(VDPR_AUTOINC, 2);
  writelong(VDPCTRL, ((address & 0x3FFF) << 16) | (address >> 14) | 0x40000000);
  int c;
  for(c = 0; c < length; c += 4)
  {
    sendcmd(CMD_WRITE | CMD_LONG | 4, VDPDATA);
    senddata(&source[c], 4);
  }
  if(c <= length - 2)
  {
    sendcmd(CMD_WRITE | CMD_WORD | 2, VDPDATA);
    senddata(&source[c], 2);
  }
}

void vramreadmem(u8 *target, u32 address, int length)
{
  // Extremely slow. Should use VRAM buffering
  vdpsetreg(VDPR_AUTOINC, 2);
  writelong(VDPCTRL, ((address & 0x3FFF) << 16) | (address >> 14));
  int c;
  for(c = 0; c < length; c += 4)
  {
    sendcmd(CMD_READ | CMD_LONG | 4, VDPDATA);
    readdata();
    if(inp[0] != (CMD_WRITE | CMD_LONG | 4))
    {
      printf("Genesis communication error. Received %02X instead of %02X.\n", inp[0], (CMD_WRITE | CMD_LONG | 4));
      exit(2);
    }
    memcpy(&target[c], &inp[4], 4);
  }
  if(c <= length - 2)
  {
    sendcmd(CMD_READ | CMD_WORD | 2, VDPDATA);
    readdata();
    if(inp[0] != (CMD_WRITE | CMD_WORD | 2))
    {
      printf("Genesis communication error. Received %02X instead of %02X.\n", inp[0], (CMD_WRITE | CMD_WORD | 2));
      exit(2);
    }
    memcpy(&target[c], &inp[2], 2);
  }
}

void sendfile(const char *token, u32 address)
{
  FILE *f = fopen(token, "r");
  if(!f)
  {
    printf("Cannot open %s\n", token);
    return;
  }
  printf("Sending %s ...\n", token);
  while(!feof(f))
  {
    u8 c[32];
    size_t s = fread(c, 1, 32, f);
    printf(" %06X -> %06X [%lu]\n", (u32)address, (u32)(address + s - 1), s);
    sendburst(address, c, s);
    address += s;
  }
  fclose(f);
}

void subsendfile(const char *path, u32 address)
{
  u8 *data;
  int size = readfile(path, &data);

  if(size > 0)
  {
    subsendmem(address, data, size);
  }
  else
  {
    fprintf(stderr, "Could not read file %s.\n", path);
    exit(3);
  }

  free(data);
}

void substop()
{
  writebyte(0xA1200E, readbyte(0xA1200E) | 0x80); // Set COMMFLAG 15
  writebyte(0xA12000, 0x01); // Trigger L2 interrupt on sub CPU
  while(!(readbyte(0xA1200F) & 0x80)); // Wait until sub CPU enters monitor mode
}

void subgo()
{
  if(readbyte(0xA1200F) & 0x80) // Test whether Sub CPU is in monitor mode
  {
    // Generate a falling edge on comm flag 15
    u8 commflag = readbyte(0xA1200E);
    writebyte(0xA1200E, commflag | 0x80);
    writebyte(0xA1200E, commflag & ~0x80);
  }
}

int submonstate = 0;

void submon()
{
  if(readbyte(0xA1200F) & 0x80) // Test whether Sub CPU is already in monitor mode
  {
    printf("Sub CPU already in monitor mode\n");
    submonstate = 1;
  }
  else
  {
    submonstate = 0;
    substop();
  }
}

void subrun()
{
  // Revert the effect of submon, if any
  if(!submonstate)
  {
    subgo();
  }
}

void subwritebyte(u32 address, u32 val)
{
  // Generate the move instruction
  u8 program[8];
  program[0] = 0x13;
  program[1] = 0xFC;
  program[2] = 0;
  program[3] = val & 0xFF;
  program[4] = 0;
  program[5] = (address >> 16) & 0xFF;
  program[6] = (address >> 8) & 0xFF;
  program[7] = address & 0xFF;
  
  submon();
  subreq();
  subsetbank(0);
  // Put program in the reserved interrupt area
  sendmem(0x020030, program, 8);

  // Put the sub CPU in trace mode
  u32 oldpc = readlong(SUBREG_PC);
  writelong(SUBREG_PC, 0x30);
  u32 oldsr = readword(SUBREG_SR);
  writeword(SUBREG_SR, 0xA700);

  subrelease();

  // Run the instruction
  subgo();
  // Wait for TRACE interrupt on sub CPU
  usleep(50);
  while(!(readbyte(0xA1200F) & 0x20));

  subreq();
  subsetbank(0);

  if(readlong(SUBREG_PC) != 0x38) {
    printf("Warning: write operation failed.\n");
  }

  // Restore sub CPU status
  writelong(SUBREG_PC, oldpc);
  writeword(SUBREG_SR, oldsr);
  subrelease();

  subrun();
}

void subwriteword(u32 address, u32 val)
{
  // Generate the move instruction
  u8 program[8];
  program[0] = 0x33;
  program[1] = 0xFC;
  program[2] = (val >> 8) & 0xFF;
  program[3] = val & 0xFF;
  program[4] = 0;
  program[5] = (address >> 16) & 0xFF;
  program[6] = (address >> 8) & 0xFF;
  program[7] = address & 0xFF;
  
  submon();
  subreq();
  subsetbank(0);
  // Put program in the reserved interrupt area
  sendmem(0x020030, program, 8);

  // Put the sub CPU in trace mode
  u32 oldpc = readlong(SUBREG_PC);
  writelong(SUBREG_PC, 0x30);
  u32 oldsr = readword(SUBREG_SR);
  writeword(SUBREG_SR, 0xA700);

  subrelease();

  // Run the instruction
  subgo();
  // Wait for TRACE interrupt on sub CPU
  usleep(50);
  while(!(readbyte(0xA1200F) & 0x20));

  subreq();
  subsetbank(0);

  if(readlong(SUBREG_PC) != 0x38) {
    printf("Warning: write operation failed.\n");
  }

  // Restore sub CPU status
  writelong(SUBREG_PC, oldpc);
  writeword(SUBREG_SR, oldsr);
  subrelease();

  subrun();
}

void subwritelong(u32 address, u32 val)
{
  // Generate the move instruction
  u8 program[10];
  program[0] = 0x23;
  program[1] = 0xFC;
  program[2] = (val >> 24) & 0xFF;
  program[3] = (val >> 16) & 0xFF;
  program[4] = (val >> 8) & 0xFF;
  program[5] = val & 0xFF;
  program[6] = (address >> 24) & 0xFF;
  program[7] = (address >> 16) & 0xFF;
  program[8] = (address >> 8) & 0xFF;
  program[9] = address & 0xFF;
  
  submon();
  subreq();
  subsetbank(0);
  // Put program in the reserved interrupt area
  sendmem(0x020030, program, 10);

  // Put the sub CPU in trace mode
  u32 oldpc = readlong(SUBREG_PC);
  writelong(SUBREG_PC, 0x30);
  u32 oldsr = readword(SUBREG_SR);
  writeword(SUBREG_SR, 0xA700);

  subrelease();

  // Run the instruction
  subgo();
  // Wait for TRACE interrupt on sub CPU
  usleep(50);
  while(!(readbyte(0xA1200F) & 0x20));

  subreq();
  subsetbank(0);

  if(readlong(SUBREG_PC) != 0x3A) {
    printf("Warning: write operation failed.\n");
  }

  // Restore sub CPU status
  writelong(SUBREG_PC, oldpc);
  writeword(SUBREG_SR, oldsr);
  subrelease();

  subrun();
}

u32 subreadbyte(u32 address)
{
  // Generate the move instruction
  u8 program[8];
  program[0] = 0x11;
  program[1] = 0xF9;
  program[2] = (address >> 24) & 0xFF;
  program[3] = (address >> 16) & 0xFF;
  program[4] = (address >> 8) & 0xFF;
  program[5] = address & 0xFF;
  program[6] = 0x00; // Data will be written at 0x00005C (reserved interrupt area)
  program[7] = 0x5C;
  
  submon();
  subreq();
  subsetbank(0);
  // Put program in the reserved interrupt area
  sendmem(0x020030, program, 8);

  // Put the sub CPU in trace mode
  u32 oldpc = readlong(SUBREG_PC);
  writelong(SUBREG_PC, 0x30);
  u32 oldsr = readword(SUBREG_SR);
  writeword(SUBREG_SR, 0xA700);

  subrelease();

  // Run the instruction
  subgo();
  // Wait for TRACE interrupt on sub CPU
  usleep(50);
  while(!(readbyte(0xA1200F) & 0x20));

  subreq();
  subsetbank(0);

  if(readlong(SUBREG_PC) != 0x38) {
    printf("Warning: read operation failed.\n");
  }

  // Restore sub CPU status
  writelong(SUBREG_PC, oldpc);
  writeword(SUBREG_SR, oldsr);

  // Read result
  u32 result = readbyte(0x02005C);
  subrelease();

  subrun();

  return result;
}

u32 subreadword(u32 address)
{
  // Generate the move instruction
  u8 program[8];
  program[0] = 0x31;
  program[1] = 0xF9;
  program[2] = (address >> 24) & 0xFF;
  program[3] = (address >> 16) & 0xFF;
  program[4] = (address >> 8) & 0xFF;
  program[5] = address & 0xFF;
  program[6] = 0x00; // Data will be written at 0x00005C (reserved interrupt area)
  program[7] = 0x5C;
  
  submon();
  subreq();
  subsetbank(0);
  // Put program in the reserved interrupt area
  sendmem(0x020030, program, 8);

  // Put the sub CPU in trace mode
  u32 oldpc = readlong(SUBREG_PC);
  writelong(SUBREG_PC, 0x30);
  u32 oldsr = readword(SUBREG_SR);
  writeword(SUBREG_SR, 0xA700);

  subrelease();

  // Run the instruction
  subgo();
  // Wait for TRACE interrupt on sub CPU
  usleep(50);
  while(!(readbyte(0xA1200F) & 0x20));

  subreq();
  subsetbank(0);

  if(readlong(SUBREG_PC) != 0x38) {
    printf("Warning: read operation failed.\n");
  }

  // Restore sub CPU status
  writelong(SUBREG_PC, oldpc);
  writeword(SUBREG_SR, oldsr);

  // Read result
  u32 result = readword(0x02005C);
  subrelease();

  subrun();

  return result;
}

u32 subreadlong(u32 address)
{
  // Generate the move instruction
  u8 program[8];
  program[0] = 0x21;
  program[1] = 0xF9;
  program[2] = (address >> 24) & 0xFF;
  program[3] = (address >> 16) & 0xFF;
  program[4] = (address >> 8) & 0xFF;
  program[5] = address & 0xFF;
  program[6] = 0x00; // Data will be written at 0x00005C (reserved interrupt area)
  program[7] = 0x5C;
  
  submon();
  subreq();
  subsetbank(0);
  // Put program in the reserved interrupt area
  sendmem(0x020030, program, 8);

  // Put the sub CPU in trace mode
  u32 oldpc = readlong(SUBREG_PC);
  writelong(SUBREG_PC, 0x30);
  u32 oldsr = readword(SUBREG_SR);
  writeword(SUBREG_SR, 0xA700);

  subrelease();

  // Run the instruction
  subgo();
  // Wait for TRACE interrupt on sub CPU
  usleep(50);
  while(!(readbyte(0xA1200F) & 0x20));

  subreq();
  subsetbank(0);

  if(readlong(SUBREG_PC) != 0x38) {
    printf("Warning: read operation failed.\n");
  }

  // Restore sub CPU status
  writelong(SUBREG_PC, oldpc);
  writeword(SUBREG_SR, oldsr);

  // Read result
  u32 result = readlong(0x02005C);
  subrelease();

  subrun();

  return result;
}

void open_device(const char *device)
{
  struct termios oldtio,newtio;

  senddata = senddata_ip;

  genfd = open(device, O_RDWR | O_NOCTTY); 
  if(genfd < 0)
  {
    fprintf(stderr, "Cannot open %s : %s\n", device, strerror(errno));
    exit(3);
  }

  tcgetattr(genfd, &oldtio);

  bzero(&newtio, sizeof(newtio));
  newtio.c_cflag = B115200 | CS8 | CLOCAL;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;
  newtio.c_lflag = 0; 
  newtio.c_cc[VTIME] = 0;
  newtio.c_cc[VMIN] = 1;

  tcflush(genfd, TCIOFLUSH);
  tcsetattr(genfd,TCSANOW,&newtio);
}

void open_network(const char *host)
{
  int res;
  struct sockaddr_in server;
  struct hostent *hp;
 
  senddata = senddata_ip;

  genfd = socket(AF_INET, SOCK_STREAM, 0);
  server.sin_family = AF_INET;
  hp = gethostbyname(host);
  if(!hp)
  {
    fprintf(stderr, "Cannot find IP address for host %s\n", host);
    exit(3);
  }
  bcopy(hp->h_addr, &(server.sin_addr.s_addr), hp->h_length);
  server.sin_port = htons(2612);
  res = connect(genfd, (struct sockaddr *) &server, sizeof(server));
  if(res == -1)
  {
    fprintf(stderr, "Cannot connect to %s : %s\n", host, strerror(errno));
    exit(3);
  }
}

void open_debugger(const char *path)
{
  struct stat s;
  if(!stat(path, &s))
  {
    // Serial device found
    open_device(path);
  }
  else
  {
    // Fallback to network connection
    open_network(path);
  }
}

