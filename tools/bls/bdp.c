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
        printf("<%02X", inp[inpl+b]);
      }
      inpl += r;
    }
  } while(inpl < packetlen(inp[0]));

  return 1;
}

void senddata_ip(const u8 *data, int size)
{
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
        printf(">%02X", data[b]);
      }
      data += w;
      size -= w;
    }
  }
}

void senddata_serial(const u8 *data, int size)
{
#define SERIAL_BUFFER 1
  while(size)
  {
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
      usleep(SERIAL_BUFFER * w * 1000000 / (115200/8));
      // Wait until buffer is empty before sending data again
      while(tcdrain(genfd) == -1 && errno == EINTR);
      int b;
      if(bdpdump) for(b = 0; b < w; ++b)
      {
        printf(">%02X", data[b]);
      }
      data += w;
      size -= w;
    }
  }
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
    substatus = readword(0xA12000);
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
      writeword(0xA12000, 0x0001); // Release busreq if bus was not taken.

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
  writeword(0xA12000, 0x0001); // Release reset high
  usleep(80000);
  writeword(0xA12000, 0x0000); // Bring reset low
  usleep(80000);
  writeword(0xA12000, 0x0001); // Release reset high
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


void subwritelong(u32 address, u32 l)
{
  subreq();

  subsetbank(address / 0x20000);
  sendcmd(CMD_WRITE | CMD_LONG | 4, (address & 0x1FFFF) + 0x20000);
  u8 d[4];
  setint(l, d, 4);
  senddata(d, 4);

  subrelease();
}

void subwriteword(u32 address, u32 w)
{
  subreq();

  subsetbank(address / 0x20000);
  sendcmd(CMD_WRITE | CMD_WORD | 2, (address & 0x1FFFF) + 0x20000);
  u8 d[2];
  setint(w, d, 2);
  senddata(d, 2);

  subrelease();
}

void subwritebyte(u32 address, u32 b)
{
  subreq();

  subsetbank(address / 0x20000);
  sendcmd(CMD_WRITE | CMD_BYTE | 1, (address & 0x1FFFF) + 0x20000);
  u8 d = b;
  senddata(&d, 1);

  subrelease();
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

