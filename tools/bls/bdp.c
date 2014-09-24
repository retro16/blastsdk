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
#include "bdp.h"

#define SERIAL_BAUDRATE B1000000
#define MAX_BREAKPOINTS 255

// Genesis command constants
#define CMD_HANDSHAKE   0x00
#define CMD_EXIT        0x20
#define CMD_READ        0x00
#define CMD_WRITE       0x20
#define CMD_BYTE        0x40
#define CMD_WORD        0xC0
#define CMD_LONG        0x80
#define CMD_LEN         0x1F

// 1M flag in ga_status
#define SCD_1M 0x00040000

// Globals
int genfd = -1;
static u8 inp[36];
static int inpl;
static int bdpdump = 0;
static int cpustate[3] = {0,0,0};
static int busstate[3] = {0,0,0};
static u32 regaddr[3] = {REGADDR, SUBREGADDR, 0};
static unknown_handler_ptr on_unknown = NULL;
static breakpoint_handler_ptr on_breakpoint = NULL;
static exception_handler_ptr on_exception = NULL;

static u32 ga_status = 0xFFFF; // Gate array status before entering subreq / wram
static u32 breakpoints[2][MAX_BREAKPOINTS + 1][2];

// Device access
void open_debugger(const char *path, unknown_handler_ptr on_unknown, breakpoint_handler_ptr on_breakpoint, exception_handler_ptr on_exception);
static void open_device(const char *device);
static void open_network(const char *host);
void close_debugger();
void bdp_set_dump(int newdump);
int bdp_readdata();
int bridgequery(u8 *data);

// Low level packet reading
static int packetlen(u8 header);
static int readdata();

// Low level packet sending
static void senddata(const u8 *data, int size);
static void waitack_tcp();
static void waitack_serial();
static void (*waitack)();
static void sendcmd(u8 header, u32 address);

// Memory read
static void readburst(u8 *target, u32 address, u32 length);
void readmem(int cpu, u8 *target, u32 address, u32 length);
void readwram(int mode, const u8 *source, u32 length); // Mode : 0 = 2M, 1 = 1M bank 0, 2 = 1M bank 1
void readvram(u8 *target, u32 address, u32 length);

// Memory write
static void writeburst(u32 address, const u8 *source, u32 length);
void writemem_verify(int cpu, u32 address, const u8 *source, u32 length);
void writemem(int cpu, u32 address, const u8 *source, u32 length);
void sendfile(int cpu, const char *filename, u32 address);
void writewram(int mode, u32 address, const u8 *source, u32 length);
void writevram(u32 address, const u8 *source, u32 length);

// Bus access
u32 readlong(int cpu, u32 address);
u32 readword(int cpu, u32 address);
u32 readbyte(int cpu, u32 address);
void writebyte(int cpu, u32 address, u32 b);
void writeword(int cpu, u32 address, u32 w);
void writelong(int cpu, u32 address, u32 l);
static void subreq();
static void subsetbank(int bank);
static void subrelease();
static void subreset();
static void busreq(int cpu, u32 address);
static void busrelease(int cpu);

// CPU access
u32 readreg(int cpu, int reg); // 0-7 = D0-D7, 8-15 = A0-A7, 16 = PC, 17 = SR
void readregs(int cpu, u32 *regs);
void setreg(int cpu, int reg, u32 value);
void setregs(int cpu, u32 *regs);
void startcpu(int cpu); // Enter run mode
void stopcpu(int cpu); // Enter monitor mode
void stepcpu(int cpu);
void resetcpu(int cpu);
static void subexec(u32 opcode, u32 op1, u32 op1size, u32 op2, u32 op2size);
int is_running(int cpu);

static void setcpustate(int cpu, int newstate);
static void cpumonitor(int cpu);
static void cpurelease(int cpu);

// Breakpoints
void setup_breakpoints(int cpu);
void cleanup_breakpoints(int cpu);
void list_breakpoints(int cpu);
void set_breakpoint(int cpu, u32 address);
int delete_breakpoint(int cpu, u32 address);
static u32 breakpoint_reached(int cpu);



void open_debugger(const char *path, unknown_handler_ptr on_unknown_callback, breakpoint_handler_ptr on_breakpoint_callback, exception_handler_ptr on_exception_callback)
{
  struct stat s;

  if(!stat(path, &s)) {
    // Serial device found
    open_device(path);
  } else {
    // Fallback to network connection
    open_network(path);
  }

  on_unknown = on_unknown_callback;
  on_breakpoint = on_breakpoint_callback;
  on_exception = on_exception_callback;
}


static void open_device(const char *device)
{
  struct termios oldtio,newtio;
  waitack = waitack_serial;
  genfd = open(device, O_RDWR | O_NOCTTY);

  if(genfd < 0) {
    fprintf(stderr, "Cannot open %s : %s\n", device, strerror(errno));
    exit(3);
  }

  tcgetattr(genfd, &oldtio);
  bzero(&newtio, sizeof(newtio));
  cfmakeraw(&newtio);
  cfsetispeed(&newtio, SERIAL_BAUDRATE);
  cfsetospeed(&newtio, SERIAL_BAUDRATE);
  newtio.c_cc[VTIME] = 0;
  newtio.c_cc[VMIN] = 1;
  tcflush(genfd, TCIOFLUSH);
  tcsetattr(genfd,TCSANOW,&newtio);
}


static void open_network(const char *host)
{
  int res;
  struct sockaddr_in server;
  struct hostent *hp;
  waitack = waitack_tcp;
  genfd = socket(AF_INET, SOCK_STREAM, 0);
  server.sin_family = AF_INET;
  hp = gethostbyname(host);

  if(!hp) {
    fprintf(stderr, "Cannot find IP address for host %s\n", host);
    exit(3);
  }

  bcopy(hp->h_addr, &(server.sin_addr.s_addr), hp->h_length);
  server.sin_port = htons(2612);
  res = connect(genfd, (struct sockaddr *) &server, sizeof(server));

  if(res == -1) {
    fprintf(stderr, "Cannot connect to %s : %s\n", host, strerror(errno));
    exit(3);
  }
}


void close_debugger()
{
  if(genfd == -1) {
    return;
  }

  close(genfd);
  genfd = -1;
}


void bdp_set_dump(int value)
{
  bdpdump = value;
}


// Call this when data is available on the file descriptor
int bdp_readdata()
{
  readdata();

  if(inpl < 4) {
    on_unknown(inp, inpl);
    return 0;
  } else if(inpl == 4 && inp[0] == 0 && inp[1] == 0 && inp[2] == 0 && inp[3] == 0) {
    // Main CPU stopped
    return 1;
  } else if(inpl == 4 && inp[0] == 0x20 && inp[1] == 0 && inp[2] == 0 && inp[3] == 0) {
    // Main CPU started
    return 1;
  } else if(inp[0] == 0x00 && inp[1] == 0x00 && inp[2] < 3 && inp[3] == 0x27) {
    u32 bpaddress;

    if((bpaddress = breakpoint_reached(inp[2]))) {
      on_breakpoint(inp[2], bpaddress);
    } else {
      on_unknown(inp, inpl);
    }
  } else if(inp[0] == 0x00 && inp[1] == 0x00 && inp[2] < 3 && inp[3] < 0x30) {
    on_exception(inp[2], inp[3]);
  } else {
    on_unknown(inp, inpl);
  }

  return 1;
}


int bridgequery(u8 *data)
{
  sendcmd(CMD_EXIT, 0x56510A);
  waitack();
  readdata();

  if(inpl != 4
      || inp[0] != 0x20
      || inp[1] < '0' || inp[1] > '9'
      || inp[2] < '0' || inp[2] > '9'
      || inp[3] < '0' || inp[3] > '9') {
    on_unknown(inp, inpl);
    return 0;
  }

  data[0] = inp[1];
  data[1] = inp[2];
  data[2] = inp[3];
  return 1;
}


// Return total packet length based on header value
static int packetlen(u8 header)
{
  if((header & 0xC0) && (header & CMD_WRITE)) {
    int l = header & CMD_LEN;

    if(!l) {
      return 36;
    }

    return l + 4;
  }

  return 4;
}


// Put data in inp[] and inpl
static int readdata()
{
  inpl = 0;

  if(bdpdump) {
    printf("<");
    fflush(stdout);
  }

  do {
    // Read the header byte, then the whole remaining packet
    ssize_t r = read(genfd, &inp[inpl], inpl ? packetlen(inp[0]) - inpl : 1);

    if(r == 0) {
      fprintf(stderr, "Could not read from genesis : connection closed.\n");
      exit(2);
    }

    if(r == -1) {
      if(errno == EAGAIN) {
        usleep(100);
      } else if(errno != EINTR) {
        fprintf(stderr, "Could not read from genesis : %s\n", strerror(errno));
        exit(2);
      }
    } else {
      int b;

      if(bdpdump) for(b = 0; b < r; ++b) {
          printf("%02X", inp[inpl+b]);
          fflush(stdout);
        }

      inpl += r;
    }
  } while(inpl < packetlen(inp[0]));

  if(bdpdump) {
    printf("\n");
  }

  return 1;
}


void senddata(const u8 *data, int size)
{
  if(bdpdump) {
    printf(">");
    fflush(stdout);
  }

  while(size) {
    ssize_t w = write(genfd, data, size);

    if(w == 0) {
      fprintf(stderr, "Could not send to genesis : connection closed.\n");
      exit(2);
    }

    if(w == -1) {
      if(errno != EINTR) {
        fprintf(stderr, "Could not send to genesis : %s\n", strerror(errno));
        exit(2);
      }
    } else {
      int b;

      if(bdpdump) for(b = 0; b < w; ++b) {
          printf("%02X", data[b]);
          fflush(stdout);
        }

      data += w;
      size -= w;
    }
  }

  if(bdpdump) {
    printf("\n");
  }
}


void waitack_tcp()
{
  // No need for ACK in TCP mode
}


void waitack_serial()
{
  for(;;) {
    // Read the header byte, then the whole remaining packet
    char c;
    ssize_t r = read(genfd, &c, 1);

    if(r == 0) {
      fprintf(stderr, "Could not read from genesis : connection closed.\n");
      exit(2);
    }

    if(r == -1) {
      if(errno == EAGAIN) {
        printf(" EAGAIN\n");
        usleep(100);
      } else if(errno != EINTR) {
        fprintf(stderr, "Could not read from genesis : %s\n", strerror(errno));
        exit(2);
      }
    } else {
      if(c != 0x3F) {
        printf("Acknowledge not received. Received %02X instead.\n", (unsigned int)(unsigned char)c);
        exit(2);
      }

      return;
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


void readburst(u8 *target, u32 address, u32 length)
{
  sendcmd(CMD_READ | CMD_BYTE | (CMD_LEN & length), address);
  waitack();
  readdata();

  if(inp[0] != (CMD_WRITE | CMD_BYTE | (CMD_LEN & length))) {
    printf("Genesis communication error. Received %02X instead of %02X.\n", inp[0], (CMD_WRITE | CMD_BYTE | (CMD_LEN & length)));
    exit(2);
  }

  memcpy(target, &inp[4], length);
}


// Reads memory as fast as possible
void readmem(int cpu, u8 *target, u32 address, u32 length)
{
  if(cpu) {
    cpumonitor(0);

    if(address + length < 0x80000) {
      while(length > 0) {
        u32 burstaddr = address & 0x1FFFF;
        u32 burstlen = 32;

        if(burstaddr > 0x1FFFE0 && burstaddr + length > 0x20000) {
          // Split over 2 banks
          burstlen = burstaddr & 0x1F;
        }

        if(burstlen > length) {
          // Remainder
          burstlen = length;
        }

        busreq(1, address);
        // Write data to PRAM window
        readburst(target, burstaddr + 0x20000, burstlen);
        target += burstlen;
        address += burstlen;
        length -= burstlen;
      }
    } else {
      // source is not PRAM, use ultra slow copy !
      while(length > 0) {
        u32 burstlen;
        u32 v;

        if(length >= 4) {
          v = readlong(cpu, address);
          setint(v, target, 4);
          burstlen = 4;
        } else if(length >= 2) {
          v = readword(cpu, address);
          setint(v, target, 2);
          burstlen = 2;
        } else {
          v = readbyte(cpu, address);
          setint(v, target, 1);
          burstlen = 1;
        }

        target += burstlen;
        address += burstlen;
        length -= burstlen;
      }
    }

    cpurelease(0);
  } else {
    while(length > 32) {
      readburst(target, address, 32);
      target += 32;
      address += 32;
      length -= 32;
    }

    readburst(target, address, length);
  }
}

/*
void readwram(int mode, const u8 *source, u32 length) // Mode : 0 = 2M, 1 = 1M bank 0, 2 = 1M bank 1
{
  // TODO
}
*/

void readvram(u8 *target, u32 address, u32 length)
{
  // Extremely slow. Should use DMA in a temporary buffer.
  cpumonitor(0);
  vdpsetreg(VDPR_AUTOINC, 2);
  writelong(0, VDPCTRL, ((address & 0x3FFF) << 16) | (address >> 14));
  u32 c;

  for(c = 0; c < length; c += 4) {
    readlong(0, VDPDATA);

    if(inpl != 8 || inp[0] != (CMD_WRITE | CMD_LONG | 4)) {
      on_unknown(inp, inpl);
    }

    memcpy(&target[c], &inp[4], 4);
  }

  c -= 4;

  if(c == length - 2) {
    readword(0, VDPDATA);

    if(inpl != 8 || inp[0] != (CMD_WRITE | CMD_LONG | 4)) {
      on_unknown(inp, inpl);
    }

    memcpy(&target[c], &inp[4], 2);
  }

  cpurelease(0);
}


// Send one data packet as a byte stream (32 bytes max)
void writeburst(u32 address, const u8 *source, u32 length)
{
  sendcmd(CMD_WRITE | CMD_BYTE | (CMD_LEN & length), address);
  senddata(source, length);
  waitack();
}


// Send data into RAM as fast as possible
void writemem(int cpu, u32 address, const u8 *source, u32 length)
{
  cpumonitor(cpu);

  if(cpu) {
    cpumonitor(0);

    if(address + length < 0x80000) {
      while(length > 0) {
        u32 burstaddr = address & 0x1FFFF;
        u32 burstlen = 32;

        if(burstaddr > 0x1FFFE0 && burstaddr + length > 0x20000) {
          // Split over 2 banks
          burstlen = burstaddr & 0x1F;
        }

        if(burstlen > length) {
          // Remainder
          burstlen = length;
        }

        busreq(1, address);
        // Write data to PRAM window
        writeburst(burstaddr + 0x20000, source, burstlen);
        source += burstlen;
        address += burstlen;
        length -= burstlen;
      }
    } else {
      // target is not PRAM, use ultra slow copy !
      while(length > 0) {
        u32 burstlen;

        if(length >= 4) {
          writelong(cpu, address, ((u32)source[0] << 24) | ((u32)source[1] << 16) | ((u32)source[2] << 8) | source[3]);
          burstlen = 4;
        } else if(length >= 2) {
          writeword(cpu, address, ((u32)source[0] << 8) | source[1]);
          burstlen = 2;
        } else {
          writebyte(cpu, address, *source);
          burstlen = 1;
        }

        source += burstlen;
        address += burstlen;
        length -= burstlen;
      }
    }

    cpurelease(0);
  } else {
    while(length > 32) {
      writeburst(address, source, 32);
      source += 32;
      address += 32;
      length -= 32;
    }

    writeburst(address, source, length);
  }

  cpurelease(cpu);
}


// Send data into RAM and verify data integrity
void writemem_verify(int cpu, u32 address, const u8 *source, u32 length)
{
  cpumonitor(cpu);

  if(cpu) {
    // TODO
  } else {
    u8 verify[32];

    while(length > 32) {
writemem_verify_retry:
      writeburst(address, source, 32);
      readburst(verify, address, 32);

      if(memcmp(source, verify, 32) != 0) {
        printf("Corrupt data at %06X. Retrying.\n", address);
        goto writemem_verify_retry;
      }

      source += 32;
      address += 32;
      length -= 32;
    }

writemem_verify_retry_last:
    writeburst(address, source, length);
    readburst(verify, address, length);

    if(memcmp(source, verify, length) != 0) {
      printf("Corrupt data at %06X. Retrying.\n", address);
      goto writemem_verify_retry_last;
    }
  }

  cpurelease(cpu);
}

void sendfile(int cpu, const char *token, u32 address)
{
  cpumonitor(cpu);

  if(cpu) {
    // TODO
  } else {
    FILE *f = fopen(token, "rb");

    if(!f) {
      printf("Cannot open %s\n", token);
      return;
    }

    printf("Sending %s ...\n", token);

    while(!feof(f)) {
      u8 c[32];
      size_t s = fread(c, 1, 32, f);
      printf(" %06X -> %06X [%lu]\n", (u32)address, (u32)(address + s - 1), s);
      writeburst(address, c, s);
      address += s;
    }

    fclose(f);
  }

  cpurelease(cpu);
}

/*
void writewram(int mode, u32 address, const u8 *source, u32 length)
{
  // TODO
}
*/

void writevram(u32 address, const u8 *source, u32 length)
{
  cpumonitor(0);
  // Extremely slow. Should use VRAM buffering
  vdpsetreg(VDPR_AUTOINC, 2);
  writelong(0, VDPCTRL, ((address & 0x3FFF) << 16) | (address >> 14) | 0x40000000);
  u32 c;

  for(c = 0; c < length; c += 4) {
    u32 v = getint(&source[c], 4);
    writelong(0, VDPDATA, v);
  }

  c -= 4;

  if(c <= length - 2) {
    u32 v = getint(&source[c], 2);
    writeword(0, VDPDATA, v);
  }

  cpurelease(0);
}


u32 readbyte(int cpu, u32 address)
{
  u32 value;
  cpumonitor(cpu);

  if(cpu) {
    cpumonitor(0);

    if(address < 0x80000) {
      // Access PRAM from main CPU
      busreq(cpu, address);
      value = readbyte(0, (address & 0x1FFFF) + 0x20000);
    } else {
      // Execute an instruction to move the value in PRAM
      // MOVE.B address, (SUB_SCRATCH+10).w
      subexec(0x11F9, address, 4, SUB_SCRATCH + 10, 2);
      value = readbyte(cpu, SUB_SCRATCH + 10);
    }

    cpurelease(0);
  } else {
    sendcmd(CMD_READ | CMD_BYTE | 1, address);
    waitack();
    readdata();

    if(inp[0] != (CMD_WRITE | CMD_BYTE | 1)) {
      printf("Genesis communication error. Received %02X instead of %02X.\n", inp[0], (CMD_WRITE | CMD_BYTE | 1));
      exit(2);
    }

    value = getint(&inp[4], 1);
  }

  cpurelease(cpu);
  return value;
}


u32 readword(int cpu, u32 address)
{
  u32 value;
  cpumonitor(cpu);

  if(cpu) {
    cpumonitor(0);

    if(address < 0x80000) {
      // Access PRAM from main CPU
      busreq(cpu, address);
      value = readword(0, (address & 0x1FFFF) + 0x20000);
    } else {
      // Execute an instruction to move the value in PRAM
      // MOVE.W address, (SUB_SCRATCH+10).w
      subexec(0x31F9, address, 4, SUB_SCRATCH + 10, 2);
      value = readword(cpu, SUB_SCRATCH + 10);
    }

    cpurelease(0);
  } else {
    sendcmd(CMD_READ | CMD_WORD | 2, address);
    waitack();
    readdata();

    if(inp[0] != (CMD_WRITE | CMD_WORD | 2)) {
      printf("Genesis communication error. Received %02X instead of %02X.\n", inp[0], (CMD_WRITE | CMD_WORD | 2));
      exit(2);
    }

    value = getint(&inp[4], 2);
  }

  cpurelease(cpu);
  return value;
}


u32 readlong(int cpu, u32 address)
{
  u32 value;
  cpumonitor(cpu);

  if(cpu) {
    cpumonitor(0);

    if(address < 0x7FFFFE && (address & 0xC0000) == ((address + 2) & 0xC0000)) {
      // Access PRAM from main CPU
      busreq(cpu, address);
      value = readlong(0, (address & 0x1FFFF) + 0x20000);
    } else {
      // Execute an instruction to move the value in PRAM
      // MOVE.W address, (SUB_SCRATCH+10).w
      subexec(0x31F9, address, 4, SUB_SCRATCH + 10, 2);
      value = readword(cpu, SUB_SCRATCH + 10);
    }

    cpurelease(0);
  } else {
    sendcmd(CMD_READ | CMD_LONG | 4, address);
    waitack();
    readdata();

    if(inp[0] != (CMD_WRITE | CMD_LONG | 4)) {
      printf("Genesis communication error. Received %02X instead of %02X.\n", inp[0], (CMD_WRITE | CMD_LONG | 4));
      exit(2);
    }

    value = getint(&inp[4], 4);
  }

  cpurelease(cpu);
  return value;
}


void writebyte(int cpu, u32 address, u32 b)
{
  cpumonitor(cpu);

  if(cpu) {
    if(address < 0x80000) {
      // Access PRAM from main CPU
      cpumonitor(0);
      busreq(cpu, address);
      writebyte(0, (address & 0x20000) + 0x20000, b);
      cpurelease(0);
    } else {
      // Execute an instruction to move the value in PRAM
      // MOVE.B #b, address.l
      subexec(0x13FC, b, 2, address, 4);
    }
  } else {
    sendcmd(CMD_WRITE | CMD_BYTE | 1, address);
    u8 d = b;
    senddata(&d, 1);
    waitack();
  }

  cpurelease(cpu);
}

void writeword(int cpu, u32 address, u32 w)
{
  cpumonitor(cpu);

  if(cpu) {
    if(address < 0x80000) {
      // Access PRAM from main CPU
      cpumonitor(0);
      busreq(cpu, address);
      writeword(0, (address & 0x20000) + 0x20000, w);
      cpurelease(0);
    } else {
      // Execute an instruction to move the value in PRAM
      // MOVE.W #w, address.l
      subexec(0x33FC, w, 2, address, 4);
    }
  } else {
    sendcmd(CMD_WRITE | CMD_WORD | 2, address);
    u8 d[2];
    setint(w, d, 2);
    senddata(d, 2);
    waitack();
  }

  cpurelease(cpu);
}

void writelong(int cpu, u32 address, u32 l)
{
  cpumonitor(cpu);

  if(cpu) {
    if(address < 0x7FFFFE && (address & 0xC0000) == ((address + 2) & 0xC0000)) {
      // Access PRAM from main CPU
      cpumonitor(0);
      busreq(cpu, address);
      writelong(0, (address & 0x20000) + 0x20000, l);
      cpurelease(0);
    } else {
      // Execute an instruction to move the value in PRAM
      // MOVE.L #l, address.l
      subexec(0x23FC, l, 4, address, 4);
    }
  } else {
    sendcmd(CMD_WRITE | CMD_LONG | 4, address);
    u8 d[4];
    setint(l, d, 4);
    senddata(d, 4);
    waitack();
  }

  cpurelease(cpu);
}


u32 readreg(int cpu, int reg)
{
  u32 address = reg * 4 + regaddr[cpu];
  u32 value;

  if(reg == 17) {
    value = readword(cpu, address);
  } else {
    value = readlong(cpu, address);
  }

  return value;
}

void readregs(int cpu, u32 *regs)
{
  u8 data[70];
  cpumonitor(cpu);
  readmem(cpu, data, regaddr[cpu], 70);
  cpurelease(cpu);
  int r;

  for(r = 0; r < 17; ++r) {
    regs[r] = (u32)data[r * 4] << 24
              | (u32)data[r * 4 + 1] << 16
              | (u32)data[r * 4 + 2] << 8
              | (u32)data[r * 4 + 3];
  }

  regs[17] = (u32)data[68] << 8 | data[69];
}


void setreg(int cpu, int reg, u32 value)
{
  u32 address = reg * 4 + regaddr[cpu];

  if(reg == 17) {
    writeword(cpu, address, value);
  } else {
    writelong(cpu, address, value);
  }
}


void setregs(int cpu, u32 *regs)
{
  u8 data[70];
  int r;

  for(r = 0; r < 17; ++r) {
    data[r * 4] = regs[r] >> 24;
    data[r * 4 + 1] = regs[r] >> 16;
    data[r * 4 + 2] = regs[r] >> 8;
    data[r * 4 + 3] = regs[r];
  }

  data[68] = regs[17] >> 8;
  data[69] = regs[17];
  cpumonitor(cpu);
  writemem(cpu, regaddr[cpu], data, 70);
  cpurelease(cpu);
}


void startcpu(int cpu)
{
  setcpustate(cpu, 0);
}


void stopcpu(int cpu)
{
  setcpustate(cpu, 1);
}


void stepcpu(int cpu)
{
  cpumonitor(cpu);
  u32 oldsr = readreg(cpu, REG_SR);
  setreg(cpu, REG_SR, 0x8700 | oldsr); // Set trace bit and mask interrupts

  // Run the CPU
  int oldstate = cpustate[cpu];
  setcpustate(cpu, 0);

  if(cpu) {
    // Wait for TRACE interrupt on sub CPU
    do {
      usleep(50);
    } while((readbyte(0, 0xA1200F) & 0xC0) != 0xC0);
  } else {
    // Wait for TRACE interrupt on main CPU
    readdata();

    if(inpl != 4
        || inp[0] != CMD_HANDSHAKE
        || inp[1] != 0x00
        || inp[2] != 0x00
        || inp[3] != 0x09) {
      on_unknown(inp, inpl);
    }
  }

  // CPU returned in its previous state
  cpustate[cpu] = oldstate;

  // Restore SR
  setreg(cpu, REG_SR, (readreg(cpu, REG_SR) & 0x00FF) | (oldsr & 0xFF00));

  cpurelease(cpu);
}


void resetcpu(int cpu)
{
  cpumonitor(cpu);

  if(cpu) {
    setup_breakpoints(cpu);
    subreset();
    cpustate[cpu] = 0;
  } else {
    cpumonitor(cpu);
    // Write RESET instruction in REG_A7 (since it will be overwritten by the operation anyway)
    setreg(cpu, REG_SP, 0x4E704E70);
    // Execute instruction at REG_A7
    setreg(cpu, REG_PC, regaddr[cpu] + REG_SP * 4);
    // Ensure nothing will interrupt the process
    setreg(cpu, REG_SR, 0x2700);
    // Restart the CPU which will execute RESET immediately after exiting monitor mode
    setcpustate(cpu, 0);
  }
}


// Generate one instruction and execute it on the sub CPU
static void subexec(u32 opcode, u32 op1, u32 op1size, u32 op2, u32 op2size)
{
  // Generate the move instruction
  u8 program[8];
  int i = 0;
  program[i++] = opcode >> 8;
  program[i++] = opcode;

  if(op1size == 4) {
    program[i++] = op1 >> 24;
    program[i++] = op1 >> 16;
    program[i++] = op1 >> 8;
    program[i++] = op1;
  } else if(op1size == 2) {
    program[i++] = op1 >> 8;
    program[i++] = op1;
  }

  if(op2size == 4) {
    program[i++] = op2 >> 24;
    program[i++] = op2 >> 16;
    program[i++] = op2 >> 8;
    program[i++] = op2;
  } else if(op2size == 2) {
    program[i++] = op2 >> 8;
    program[i++] = op2;
  }

  cpumonitor(0);
  cpumonitor(1);
  busreq(1, SUB_SCRATCH);
  // Put program in the reserved interrupt area
  writemem(1, SUB_SCRATCH, program, 2 + op1size + op2size);
  // Put the sub CPU in trace mode and execute scratch
  u32 oldpc = readreg(1, REG_PC);
  setreg(1, REG_PC, SUB_SCRATCH);
  u32 oldsr = readreg(1, REG_SR);
  setreg(1, REG_SR, 0xA700);
  // Run the instruction
  int curstate = cpustate[1];
  setcpustate(1, 0);

  // Wait for TRACE interrupt on sub CPU
  do {
    usleep(50);
  } while((readbyte(0, 0xA1200F) & 0xC0) != 0xC0);

  // CPU returned in its previous state
  cpustate[1] = curstate;

  // Check that the instruction has been executed successfully
  if(readreg(1, REG_PC) != SUB_SCRATCH + 2 + op1size + op2size) {
    printf("Warning: exec failed, PC is not at the right place.\n");
  }

  // Restore sub CPU status
  setreg(1, REG_PC, oldpc);
  setreg(1, REG_SR, oldsr);
  cpurelease(1);
  cpurelease(0);
}


int is_running(int cpu)
{
  return cpustate[cpu];
}


void setcpustate(int cpu, int newstate)
{
  if(!cpustate[cpu] != !newstate) {
    if(newstate) {
      if(cpu) {
        cpumonitor(0);
        writebyte(0, 0xA1200E, readbyte(0, 0xA1200E) | 0x80); // Set COMMFLAG 15
        writebyte(0, 0xA12000, 0x01); // Trigger L2 interrupt on sub CPU

        while(!(readbyte(0, 0xA1200F) & 0x80)); // Wait until sub CPU enters monitor mode

        cpurelease(0);
      } else {
        sendcmd(CMD_HANDSHAKE, 0);
        waitack();
        readdata();

        if(inpl != 4 || inp[0] != 0x00 || inp[1] != 0x00 || inp[2] != 0x00 || inp[3] != 0x00) {
          on_unknown(inp, inpl);
        }
      }

      cleanup_breakpoints(cpu);
    } else {
      setup_breakpoints(cpu);
      busrelease(cpu);

      if(cpu) {
        // Generate a falling edge on comm flag 15
        cpumonitor(0);
        u8 commflag = readbyte(0, 0xA1200E);
        writebyte(0, 0xA1200E, commflag | 0x80);
        writebyte(0, 0xA1200E, commflag & ~0x80);
        cpurelease(0);
      } else {
        sendcmd(CMD_EXIT, 0);
        waitack();
        readdata();

        if(inpl != 4 || inp[0] != 0x20 || inp[1] != 0x00 || inp[2] != 0x00 || inp[3] != 0x00) {
          on_unknown(inp, inpl);
        }
      }
    }
  }

  cpustate[cpu] = newstate;
}


static void cpumonitor(int cpu)
{
  setcpustate(cpu, cpustate[cpu] + 1);
}


static void cpurelease(int cpu)
{
  setcpustate(cpu, cpustate[cpu] - 1);
}


static void subreq()
{
  ga_status = readbyte(0, 0xA12001);
  ga_status |= readword(0, 0xA12002) << 16;
  writeword(0, 0xA12000, 0x0003);
}


static void subsetbank(int bank)
{
  if(ga_status == 0xFFFF) {
    // This should never happen
    subreq();
  }

  if(ga_status & SCD_1M) {
    writeword(0, 0xA12002, ((bank - 1) << 6) | 0x0002);
  } else {
    writeword(0, 0xA12002, (bank - 1) << 6);
  }
}


void subrelease()
{
  if(!(ga_status & 0x0002)) {
    writebyte(0, 0xA12001, 0x01);  // Release busreq if bus was not taken.
  }

  if(ga_status & SCD_1M) {
    writeword(0, 0xA12002, (ga_status >> 16 & 0xFFC0) | 0x0002);
  } else {
    writeword(0, 0xA12002, ga_status >> 16 & 0xFFC0);
  }

  ga_status = 0xFFFF;
}


void subreset()
{
  usleep(1000);
  writebyte(0, 0xA12001, 0x01); // Release reset high
  usleep(20000);
  writebyte(0, 0xA12001, 0x00); // Bring reset low
  usleep(20000);
  writebyte(0, 0xA12001, 0x01); // Release reset high
  usleep(1000);
}


// Switch bank to access address
void busreq(int cpu, u32 address)
{
  if(cpu) {
    int bank = (address >> 17) + 1;

    if(!busstate[cpu]) {
      subreq();
    }

    if(busstate[cpu] != bank) {
      subsetbank(bank);
    }

    busstate[cpu] = bank;
  } else {
    // No busreq on main cpu
  }
}


void busrelease(int cpu)
{
  if(busstate[cpu]) {
    if(cpu) {
      subrelease();
    } else {
      // No busreq on main cpu
    }
  }

  busstate[cpu] = 0;
}


void vdpsetreg(int reg, u8 value)
{
  writeword(0, VDPCTRL, 0x8000 | ((u32)reg << 8) | value);
}


void setup_breakpoints(int cpu)
{
  int b;

  if(!breakpoints[cpu][0][0]) {
    return;
  }

  for(b = 0; b < MAX_BREAKPOINTS; ++b) {
    if(!breakpoints[cpu][b][0]) {
      break;
    }

    breakpoints[cpu][b][1] = readword(cpu, breakpoints[cpu][b][0]);
    writeword(cpu, breakpoints[cpu][b][0], 0x4E47);
  }
}


void cleanup_breakpoints(int cpu)
{
  int b;

  if(!breakpoints[cpu][0][0]) {
    return;
  }

  for(b = 0; b < MAX_BREAKPOINTS; ++b) {
    if(!breakpoints[cpu][b][0]) {
      break;
    }

    // Replace the breakpoint by the original instruction
    writeword(cpu, breakpoints[cpu][b][0], breakpoints[cpu][b][1]);
  }
}


void list_breakpoints(int cpu)
{
  // List breakpoints
  int b;

  for(b = 0; b < MAX_BREAKPOINTS; ++b) {
    if(!breakpoints[cpu][b][0]) {
      break;
    }

    printf("%s CPU breakpoint at $%06X\n", cpu ? "Sub" : "Main", breakpoints[cpu][b][0]);
  }
}


void set_breakpoint(int cpu, u32 address)
{
  int b;
  cpumonitor(cpu);

  for(b = 0; b < MAX_BREAKPOINTS; ++b) {
    if(!breakpoints[cpu][b][0]) {
      printf("Put a breakpoint at $%06X\n", address);
      breakpoints[cpu][b][0] = address;
      breakpoints[cpu][b+1][0] = 0;
      cpurelease(cpu);
      return;
    }
  }

  printf("Error: cannot place breakpoint: breakpoint count limit reached.\n");
  cpurelease(cpu);
}


int delete_breakpoint(int cpu, u32 address)
{
  int b;
  int deleted = 0;
  cpumonitor(cpu);

  for(b = 0; b < MAX_BREAKPOINTS; ++b) {
    if(!breakpoints[cpu][b][0]) {
      break;
    }

    while(breakpoints[cpu][b][0] == address) {
      int nb;

      for(nb = b; nb < MAX_BREAKPOINTS; ++nb) {
        if(!breakpoints[cpu][nb][0]) {
          break;
        }

        breakpoints[cpu][nb][0] = breakpoints[cpu][nb+1][0];
        breakpoints[cpu][nb][1] = breakpoints[cpu][nb+1][1];
      }

      deleted = 1;
    }
  }

  cpurelease(cpu);
  return deleted;
}


// Returns PC if the CPU really reached a breakpoint, 0 otherwise
u32 breakpoint_reached(int cpu)
{
  u32 pc;
  pc = readreg(cpu, REG_PC) - 2;
  int b;

  for(b = 0; b < MAX_BREAKPOINTS; ++b) {
    if(!breakpoints[cpu][b][0]) {
      break;
    }

    if(breakpoints[cpu][b][0] == pc) {
      // We hit a breakpoint and not a simple TRAP #7
      cleanup_breakpoints(cpu);
      cpustate[cpu] = 1;
      // Place PC on the instruction
      setreg(cpu, REG_PC, pc);
      return pc;
    }
  }

  return 0;
}

// vim: ts=2 sw=2 sts=2 et
