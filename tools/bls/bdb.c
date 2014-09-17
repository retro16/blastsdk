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
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/ioctl.h>

#include "bls.h"

////// Globals

sigjmp_buf mainloop_jmp;
#define MAX_BREAKPOINTS 1024

char regname[][3] = { "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "pc", "sr"};


int breakpoints[MAX_BREAKPOINTS][2] = {{-1, -1}};

////// Extern functions


void usage()
{
  fprintf(stderr, "Usage: bdb DEVICE\n"
                  "The Blast ! debugger.\n"
                  "Communicates to a SEGA Mega Drive / Genesis via an arduino.\n"
                  "\n"
                  "Options:"
                  "\n"
                  "      DEVICE   either a serial device (/dev/xxx) or an IP address.\n"
  );
}

void goto_mainloop(int sig)
{
  (void)sig;
  printf("SIGINT\n");
  rl_free_line_state();
  rl_reset_after_signal();
  
  // Go back to main loop
  siglongjmp(mainloop_jmp, 0);
}

void cleanup_breakpoints()
{
  int b;
  for(b = 0; b < MAX_BREAKPOINTS; ++b)
  {
    if(breakpoints[b][0] == -1)
    {
      break;
    }

    // Write original instruction
    writeword(breakpoints[b][0], breakpoints[b][1]);
  }
}

void print_incoming()
{
  // The genesis sent data : print it to stdout
  readdata();
  if(inpl < 4)
  {
    printf("Received partial data from the genesis\n");
  }
  else if(inp[0] == 0x00 && inp[1] == 0x00 && inp[2] == 0x00 && inp[3] >= 0x20 && inp[3] < 0x2F)
  {
    printf("TRAP #%02d\n", inp[3] - 0x20);
  }
  else if(inp[0] == 0x00 && inp[1] == 0x00 && inp[2] == 0x00 && inp[3] < 0x20)
  {
    printf("Exception %02X triggered\n", (unsigned int)(unsigned char)(inp[3]));
  }
  else if(inp[0] <= 3)
  {
    fwrite(&inp[1], 1, inp[0] & 0x03, stdout);
  }
  else
  {
    printf("(extra) ");
    hexdump(inp, inpl, 0);
  }
}

void incoming_packet()
{
  char *lb = rl_line_buffer;
  while(*(lb++)) { usleep(1000000); printf("\b \b\b"); } // Erase readline line
  printf("\b\b\b\b\b\b      \b\b\b\b\b\b"); // Erase "bdb > " prompt

  print_incoming();

  printf("bdb > %s", rl_line_buffer); // Restore readline line display
  fflush(stdout);
}

void breakpoint_interrupt(int sig)
{
  (void) sig;

  cleanup_breakpoints();
  signal(SIGINT, goto_mainloop);
  goto_mainloop(sig);
}

void parse_word(char *token, const char **line)
{
  skipblanks(line);
  while(**line > ' ')
  {
    *token = **line;
    ++token;
    ++*line;
  }
  *token = '\0';
  skipblanks(line);
}

void parse_token(char *token, const char **line)
{
  skipblanks(line);
  while(
    (**line >= 'a' && **line <= 'z')
  ||(**line >= 'A' && **line <= 'Z')
  ||(**line >= '0' && **line <= '9'))
  {
    *token = **line;
    ++token;
    ++*line;
  }
  *token = '\0';
  skipblanks(line);
}

void showreg(u32 regaddr)
{
  u8 regs[17*4 + 2];
  readmem(regs, regaddr, sizeof(regs));

  int r;
  int t;
  for(t = 0; t <= 8; t += 8)
  {
    for(r = 0; r < 8; ++r)
    {
      u32 value = getint(&regs[(r + t) * 4], 4);
      printf("%s:%08X ", regname[r + t], value);
    }
    printf("\n");
  }
  printf("pc:%08X sr:%04X\n", getint(&regs[16 * 4], 4), getint(&regs[17 * 4], 2));
}

void boot_cd(u8 *image, int imgsize)
{
  (void)imgsize;

  const u8 *ip_start;
  int ipsize;
  ipsize = getipoffset(image, &ip_start);

  const u8 *sp_start;
  int spsize;
  spsize = getspoffset(image, &sp_start);

  u32 seccodesize = detect_region(image);

  // TODO : upload BIOS simulator and setup simulator callback

  printf("CD-ROM image. IP=%04X-%04X (%d bytes). SP=%04X-%04X (%d bytes).\n", (u32)(ip_start-image), (u32)(ip_start - image + ipsize - 1), ipsize, (u32)(sp_start - image), (u32)(sp_start - image + spsize - 1), spsize);

  printf("Uploading IP (%d bytes) ...\n", ipsize);
  sendmem(0xFF0000 + seccodesize, ip_start, ipsize);

  usleep(1000); // Wait to ensure that buffers are empty

  printf("Requesting sub CPU ...\n");
  subreq();

  printf("Uploading SP (%d bytes) ...\n", spsize);
  subsendmem(0x6000 + SPHEADERSIZE, sp_start, spsize);
  printf("Set sub CPU reset vector to beginning of sub code.\n");
  subsetbank(0);
  writelong(0x020004, 0x6000 + SPHEADERSIZE);
  printf("Set main CPU registers.\n%06X PC=%08X\n%06X SP=%08X\n%06X SR=%04X\n", REG_PC, 0xFF0000 + seccodesize, REG_SP, 0xFFD000, REG_SR, 0x2700);
  writelong(REG_PC, 0xFF0000 + seccodesize);
  writelong(REG_SP, 0xFFD000);
  writeword(REG_SR, 0x2700);

  printf("Resetting sub CPU.\n");
  subreset();

  printf("Ready to boot.\n");
}

void boot_sp(u8 *image, int imgsize)
{
  (void)imgsize;

  const u8 *sp_start;
  int spsize;
  spsize = getspoffset(image, &sp_start);

  printf("CD-ROM image. SP=%04X-%04X (%d bytes).\n", (u32)(sp_start - image), (u32)(sp_start - image + spsize - 1), spsize);

  printf("Requesting sub CPU ...\n");
  subreq();

  printf("Uploading SP (%d bytes) ...\n", spsize);
  subsendmem(0x6000 + SPHEADERSIZE, sp_start, spsize);
  printf("Set sub CPU reset vector to beginning of sub code.\n");
  subsetbank(0);
  writelong(0x020004, 0x6000 + SPHEADERSIZE);

  printf("Resetting sub CPU.\n");
  subreset();

  printf("New SP code running.\n");
}

// v is the vector offset (0x08-0xBC)
// value is the target of the vector
void gen_setvector(u32 v, u32 value)
{
  if(v == 0x70)
  {
    // LEVEL4/HBLANK interrupt
    char romid[17];
    romid[16] = '\0';
    readmem((u8*)romid, 0x120, 16);
    if(strstr(romid, "BOOT ROM"))
    {
      printf("Sega CD detected : write the HBLANK vector at the special address\n");
      if((value & 0x00FF0000) != 0x00FF0000)
      {
        printf("Warning : cannot set HBLANK outside main RAM for Sega CD.\n");
      }
      writelong(0xA12006, value & 0xFFFF);
      return;
    }
  }

  // Fetch wrapper from genesis
  u32 wrapper = readlong(v) & 0x00FFFFFF;
  if(wrapper < 0x200000)
  {
    // Not in RAM : cannot change vector
    printf("Warning : cannot set vector %02X : target not in RAM.\n", v);
    return;
  }

  printf("Set interrupt vector code %02X at %06X, jumping to %06X\n", v, wrapper, value);
  writeword(wrapper, 0x4EF9); // Write jmp instruction
  writelong(wrapper + 2, value); // Write target address
}

void boot_ram(u8 *image, int imgsize)
{
  if(imgsize != 0xFF00)
  {
    printf("Incorrect image size (must be 0xFF00)\n");
    return;
  }

  u32 sp = getint(image, 4);
  u32 pc = getint(image + 0xFC, 4);

  u32 v;
  for(v = 0x08; v < 0xC0; v += 4)
  {
    u32 value = getint(image + v, 4);
    if(value != pc && value >= 0x200)
    {
      if(v == 0x68)
      {
        printf("Warning : Image overrides level 2 (BDA/pad) interrupt\n");
      }
      gen_setvector(v, value);
    }
  }

  // Upload RAM program
  sendmem(0xFF0000, image + 0x200, imgsize - 0x200);

  // Set SR, SP and PC
  writelong(REG_SP, sp);
  writelong(REG_PC, pc);
  writeword(REG_SR, 0x2700);

  printf("Ready to boot.\n");
}

void boot_img(const char *filename)
{
  u8 *image;
  int imgsize = readfile(filename, &image);
  if(imgsize < 0x200)
  {
    printf("Image too small.\n");
    free(image);
    return;
  }
  switch(getimgtype(image, imgsize))
  {
    case 0:
      printf("Invalid image type.\n");
      break;
    case 1:
      printf("Cannot boot ROM cartridges.\n");
      break;
    case 2:
      boot_cd(image, imgsize);
      break;
    case 4:
      boot_ram(image, imgsize);
      break;
  }
  free(image);

  showreg(REGADDR);
}

void boot_sp_from_iso(const char *filename)
{
  u8 *image;
  int imgsize = readfile(filename, &image);
  if(imgsize < 0x200)
  {
    printf("Image too small.\n");
    free(image);
    return;
  }
  switch(getimgtype(image, imgsize))
  {
    case 0:
    case 1:
    case 3:
    case 4:
      printf("Invalid image type.\n");
      break;
    case 2:
      boot_sp(image, imgsize);
      break;
  }
  free(image);
}

int parse_register(const char **line)
{
  skipblanks(line);
  int r = -1;
  if(**line == 's' || **line == 'S')
  {
    ++*line;
    if(**line == 'r' || **line == 'R')
    {
      ++*line;
      return 17;
    }
  }
  else if(**line == 'p' || **line == 'P')
  {
    ++*line;
    if(**line == 'c' || **line == 'C')
    {
      ++*line;
      return 16;
    }
  }
  else if(**line == 'd' || **line == 'D') r = 0;
  else if(**line == 'a' || **line == 'A') r = 8;

  if(r == -1) return -1;

  ++*line;
  if(**line >= '0' && **line <= '7')
  {
    r += **line - '0';
    ++*line;
    skipblanks(line);
    return r;
  }
  return -1;
}

// Assemble a file and send it to a memory buffer
u32 asmfile(const char *filename, u8 *target, u32 org)
{
  char cmdline[4096];
  snprintf(cmdline, 4096, "asmx -w -e -b 0x%08X -l /dev/stderr -o /dev/stdout -C 68000 %s", org, filename);
  printf("%s\n", cmdline);
  FILE *a = popen(cmdline, "r");
  u32 codesize = 0;
  while(!feof(a))
  {
    u32 readsize = fread(target, 1, 4096, a);
    codesize += readsize;
    target += readsize;
  }
  if(pclose(a))
    printf("Warning : asmx returned an error\n");
  return codesize;
}

void d68k_instr(int sub)
{
  u32 address = readlong(sub ? SUBREG_PC : REG_PC);
  u8 data[10];
  if(sub)
    subreadmem(data, address, 10);
  else
    readmem(data, address, 10);

  char out[256];
  int suspicious;
  d68k(out, 256, data, 10, 1, address, 0, 1, &suspicious);
  printf("(next) %s", out);
}

void d68k_skip_instr(int sub)
{
  if(sub) subsetbank(0);
  u32 address = readlong(sub ? SUBREG_PC : REG_PC);
  u8 data[10];
  if(sub)
    subreadmem(data, address, 10);
  else
    readmem(data, address, 10);

  char out[256];
  int suspicious;
  address = d68k(out, 256, data, 10, 1, address, 0, 1, &suspicious);
  printf("(skip) %s", out);
  if(sub) subsetbank(0);
  writelong(sub ? SUBREG_PC : REG_PC, address);
  showreg(sub ? SUBREGADDR : REGADDR);
  d68k_instr(sub);
}

char lastline[1024] = "";
void on_line_input(char *l);

int main(int argc, char **argv)
{
  if(argc < 2)
  {
    fprintf(stderr, "Missing parameter.\n");
    usage();
    exit(1);
  }

  open_debugger(argv[1]);

	printf("Blast ! debugger. Connected to %s\n", argv[1]);

  if(argc >= 3)
  {
    printf("\n");
    boot_img(argv[2]);
    sendcmd(CMD_EXIT, 0);
    waitack();
    readdata();
  }

  sigsetjmp(mainloop_jmp, 1);
  signal(SIGINT, SIG_DFL);
  rl_callback_handler_install("bdb > ", on_line_input);
  
  for(;;) {
    fd_set readset;
    int selectresult;

    FD_ZERO(&readset);
    FD_SET(0, &readset);
    FD_SET(genfd, &readset);
    
    selectresult = select(genfd + 1, &readset, NULL, NULL, NULL);
    
    if(selectresult > 0) {
      if(FD_ISSET(0, &readset)) {
        rl_callback_read_char();
      } else if(FD_ISSET(genfd, &readset)) {
        incoming_packet();
      }
    }
  }
}

void on_line_input(char *line)
{
  char token[1024] = "";
  const char *l = line;
  
  rl_callback_handler_remove();
  
  do {
    if(*l)
    {
      strcpy(lastline, l);
      add_history(l);
    }
    else
    {
      // Replay last line
      if(!lastline[0])
      {
        continue;
      }
      l = lastline;
    }

    signal(SIGINT, goto_mainloop); // Handle Ctrl-C : jump back to prompt
    
    parse_token(token, &l);

    if(strcmp(token, "q") == 0)
    {
      exit(0);
    }

    if(strcmp(token, "flush") == 0)
    {
      readdata();
      hexdump(inp, inpl, 0);

      continue;
    }

    if(strcmp(token, "asmx") == 0)
    {
      parse_word(token, &l);
      u32 address = parse_int(&l, 8);

      if(!address) address = 0xFF0000;

      u8 obj[65536];
      u32 osize = asmfile(token, obj, address);

      sendmem(address, obj, osize);
      writelong(REG_PC, address);
      showreg(REGADDR);

      continue;
    }

    if(strcmp(token, "subasmx") == 0)
    {
      parse_word(token, &l);
      u32 address = parse_int(&l, 8);

      if(!address) address = 0x006028;

      u8 obj[65536];
      u32 osize = asmfile(token, obj, address);

      substop();
      subreq();
      subsendmem(address, obj, osize);
      subsetbank(0);
      writelong(SUBREG_PC, address);
      showreg(SUBREGADDR);
      subrelease();

      continue;
    }

    if(strcmp(token, "flush") == 0)
    {
      readdata();
      hexdump(inp, inpl, 0);

      continue;
    }

    if(strcmp(token, "sr") == 0)
    {
      parse_token(token, &l);
      u32 address = parse_int(&l, 8);
      u32 value;
      switch(token[0])
      {
        case 'b':
          value = subreadbyte(address);
          break;
        case 'w':
          value = subreadword(address);
          break;
        case 'l':
          value = subreadlong(address);
          break;
      }
      printf("%06X: %08X\n", address, value);
      continue;
    }

    if(strcmp(token, "sw") == 0)
    {
      parse_token(token, &l);
      u32 address = parse_int(&l, 8);
      u32 value = parse_int(&l, 8);
      switch(token[0])
      {
        case 'b':
          subwritebyte(address, value);
          break;
        case 'w':
          subwriteword(address, value);
          break;
        case 'l':
          subwritelong(address, value);
          break;
      }
      continue;
    }

    if(strcmp(token, "r") == 0)
    {
      u8 cmd = CMD_READ;
      parse_token(token, &l);
      const char *c = &token[1];

      if(!token[0])
      {
        continue;
      }

      switch(token[0])
      {
        case 'b':
        cmd |= CMD_BYTE;
        if(token[1]) {
          cmd |= parse_int(&c, 8) & CMD_LEN;
        } else {
          cmd |= 1;
        }
        break;

        case 'w':
        cmd |= CMD_WORD;
        if(token[1]) {
          cmd |= (parse_int(&c, 8) << 1) & CMD_LEN;
        } else {
          cmd |= 2;
        }
        break;

        case 'l':
        cmd |= CMD_LONG;
        if(token[1]) {
          cmd |= (parse_int(&c, 8) << 2) & CMD_LEN;
        } else {
          cmd |= 4;
        }
        break;

        default:
        break;
      }

      u32 address = parse_int(&l, 8);
      sendcmd(cmd, address);
      waitack();

      readdata();
      if(inpl <= 4)
      {
        printf("Invalid response length : %d\n", inpl);
        hexdump(inp, inpl, 0);
        continue;
      }
      address = getint(&inp[1], 3);
      hexdump(&inp[4], inpl - 4, address);

      continue;
    }

    if(strcmp(token, "w") == 0)
    {
      parse_token(token, &l);

      u32 address = parse_int(&l, 8);

      u32 value = parse_int(&l, 8);
      switch(token[0])
      {
        case 'b':
        writebyte(address, value);
        break;

        case 'w':
        writeword(address, value);
        break;

        case 'l':
        writelong(address, value);
        break;
      }

      continue;
    }

    if(strcmp(token, "sendfile") == 0)
    {
      u32 address = parse_int(&l, 8);
      parse_word(token, &l);
      sendfile(token, address);

      continue;
    }

    if(strcmp(token, "go") == 0)
    {
      int b;
      for(b = 0; b < MAX_BREAKPOINTS; ++b)
      {
        if(breakpoints[b][0] == -1)
        {
          break;
        }
        breakpoints[b][1] = readword(breakpoints[b][0]);

        // Write a TRAP #07 instruction
        writeword(breakpoints[b][0], 0x4E47);
        signal(SIGINT, breakpoint_interrupt);
      }
      sendcmd(CMD_EXIT, 0);
      waitack();
      
      do {
        readdata();
        if((inp[0] & 0xE0) == 0 && inp[0] & 0x03)
        {
          // The genesis sent data
          fwrite(&inp[1], 1, inp[0] & 0x03, stdout);
          fflush(stdout);
        }
      } while((inp[0] & 0xE0) == 0 && (inp[0] & 0x03));
      if((inp[0] & 0xE0) != CMD_EXIT)
      {
        printf("Error : genesis did not acknowledge.\n");
        hexdump(inp, inpl, 0);
        continue;
      }

      if(breakpoints[0][0] != -1)
      {
        // Wait until breakpoint
        readdata();
        signal(SIGINT, goto_mainloop);
        if(inp[3] == 0x07)
          printf("Breakpoint triggered.\n");
        else
          printf("Interrupted before reaching breakpoint.\n");

        // Restore original instructions
        cleanup_breakpoints();

        // Adjust PC to point on the break instruction
        writelong(REG_PC, readlong(REG_PC) - 2);

        showreg(REGADDR);
      }

      continue;
    }

    if(strcmp(token, "ping") == 0)
    {
      printf("Ping");
      fflush(stdout);

      sendcmd(CMD_HANDSHAKE, 0);
      printf(" ..");
      fflush(stdout);
      waitack();
      printf(". ");
      fflush(stdout);

      readdata();
      if(inpl != 4 || inp[0])
      {
        printf("crash !\nError : genesis did not handshake.\n");
        hexdump(inp, inpl, 0);
        continue;
      }
      else
      {
        printf("pong !\n");
        hexdump(inp, inpl, 0);
      }

      continue;
    }

    if(strcmp(token, "test") == 0)
    {
      printf("Test");
      fflush(stdout);

      sendcmd(CMD_EXIT, 0x56510A);
      waitack();

      printf(" ... ");
      fflush(stdout);

      readdata();
      if(inpl != 4
      || inp[0] != 0x20
      || inp[1] < '0' || inp[1] > '9'
      || inp[2] < '0' || inp[2] > '9'
      || inp[3] < '0' || inp[3] > '9')
      {
        printf("crash !\n");
        hexdump(inp, inpl, 0);
        continue;
      }
      else
      {
        printf("OK !\n\n Protocol v%c\n Implementation v%c.%c\n", inp[1], inp[2], inp[3]);
      }

      continue;
    }

    if(strcmp(token, "reg") == 0)
    {
      showreg(REGADDR);

      continue;
    }

    if(strcmp(token, "subreg") == 0)
    {
      submon();
      subreq();
      subsetbank(0);
      showreg(SUBREGADDR);
      subrelease();
      subrun();

      continue;
    }

    if(strcmp(token, "dump") == 0)
    {
      u32 address = parse_int(&l, 12);
      skipblanks(&l);
      u32 size = parse_int(&l, 12);
      skipblanks(&l);
      u8 *data = (u8 *)malloc(size);
      readmem(data, address, size);

      skipblanks(&l);

      if(*l)
      {
        FILE *f = fopen(l, "w");
        if(!f)
        {
          printf("Cannot open file %s.\n", l);
          continue;
        }

        fwrite(data, 1, size, f);
        fclose(f);
      }
      else
      {
        hexdump(data, size, address);
      }
      continue;
    }

    if(strcmp(token, "subdump") == 0)
    {
      u32 address = parse_int(&l, 12);
      skipblanks(&l);
      u32 size = parse_int(&l, 12);
      skipblanks(&l);
      u8 *data = (u8 *)malloc(size);
      subreadmem(data, address, size);

      skipblanks(&l);

      if(*l)
      {
        FILE *f = fopen(l, "w");
        if(!f)
        {
          printf("Cannot open file %s.\n", l);
          continue;
        }

        fwrite(data, 1, size, f);
        fclose(f);
      }
      else
      {
        hexdump(data, size, address);
      }
      continue;
    }

    if(strcmp(token, "showchr") == 0)
    {
      u32 address = parse_int(&l, 12) * 32;
      u8 data[32];
      vramreadmem(data, address, 32);
      int c, x, y;
      c = 0;
      printf(" --------\n");
      for(y = 0; y < 8; ++y)
      {
        printf("|");
        for(x = 0; x < 4; ++x)
        {
          u8 c1 = data[c] >> 4;
          u8 c2 = data[c] & 0xF;
          printf("%c", c1 ? (c1 > 9 ? c1 - 10 + 'A' : c1 + '0') : ' ');
          printf("%c", c2 ? (c2 > 9 ? c2 - 10 + 'A' : c2 + '0') : ' ');
          ++c;
        }
        printf("|\n");
      }
      printf(" --------\n");
    }

    if(strcmp(token, "vdump") == 0)
    {
      u32 address = parse_int(&l, 12);
      skipblanks(&l);
      u32 size = parse_int(&l, 12);
      skipblanks(&l);
      u8 *data = (u8 *)malloc(size);
      vramreadmem(data, address, size);

      skipblanks(&l);

      if(*l)
      {
        FILE *f = fopen(l, "w");
        if(!f)
        {
          printf("Cannot open file %s.\n", l);
          continue;
        }

        fwrite(data, 1, size, f);
        fclose(f);
      }
      else
      {
        hexdump(data, size, address);
      }
      continue;
    }

    if(strcmp(token, "blsgen") == 0)
    {
      system("blsgen");
      continue;
    }

    if(strcmp(token, "d68k") == 0)
    {
      u32 address = parse_int(&l, 12);
      u32 size = parse_int(&l, 12);
      int instructions = -1;
      if(!address) { address = readlong(REG_PC); size = 0; printf("Starting at PC (%08X)\n", address); }
      if(!size) {
        size = 10;
        instructions = 1;
      }
      u8 *data = (u8 *)malloc(size);
      readmem(data, address, size);

      char out[262144];
      int suspicious;
      d68k(out, 262144, data, size, instructions, address, 1, 1, &suspicious);
      printf("%s\n", out);

      continue;
    }

    if(strcmp(token, "subd68k") == 0)
    {
      u32 address = parse_int(&l, 12);
      u32 size = parse_int(&l, 12);
      int instructions = -1;
      u8 *data = (u8 *)malloc(size);

      subreq();
      subsetbank(0);
      if(!address) { address = subreadlong(SUBREG_PC); size = 0; printf("Starting at PC (%08X)\n", address); }
      if(!size) {
        size = 10;
        instructions = 1;
      }
      subreadmem(data, address, size);
      subrelease();

      char out[262144];
      int suspicious;
      d68k(out, 262144, data, size, instructions, address, 1, 1, &suspicious);
      printf("%s\n", out);

      continue;
    }

    if(strcmp(token, "substop") == 0)
    {
      substop();
      continue;
    }

    if(strcmp(token, "subgo") == 0)
    {
      subgo();
      continue;
    }

    if(strcmp(token, "set") == 0)
    {
      int reg = parse_register(&l);
      if(reg == -1)
      {
        printf("Unknown register.\n");
        continue;
      }
      u32 value = parse_int(&l, 12);
      if(reg == 17)
      {
        writeword(REG_SR, value);
      }
      else
      {
        writelong(REGADDR + reg * 4, value);
      }
      showreg(REGADDR);
      continue;
    }

    if(strcmp(token, "subset") == 0)
    {
      int reg = parse_register(&l);
      if(reg < 0)
      {
        printf("Unknown register.\n");
        continue;
      }
      u32 value = parse_int(&l, 12);
      submon();
      subreq();
      subsetbank(0);
      if(reg == 17)
      {
        writeword(SUBREG_SR, value);
      }
      else
      {
        writelong(SUBREGADDR + reg * 4, value);
      }
      showreg(SUBREGADDR);
      subrelease();
      subrun();
      continue;
    }

    if(strcmp(token, "step") == 0 || strcmp(token, "s") == 0)
    {
      int oldsr = readword(REG_SR);
printf("oldsr=%04X\n", oldsr);
      writeword(REG_SR, 0x8700 | oldsr); // Set trace bit and mask interrupts

      sendcmd(CMD_EXIT, 0);
      waitack();
      readdata();
      readdata();
printf("SR=%04X\n", readword(REG_SR));
      writeword(REG_SR, (readword(REG_SR) & 0x00FF) | (oldsr & 0xFF00));
      showreg(REGADDR);
      d68k_instr(0);
      continue;
    }

    if(strcmp(token, "skip") == 0)
    {
      d68k_skip_instr(0);
      continue;
    }

    if(strcmp(token, "substep") == 0 || strcmp(token, "ss") == 0)
    {
      substop();
      subreq();
      subsetbank(0);
      int oldsr = readword(SUBREG_SR);
      writeword(SUBREG_SR, 0x8700 | oldsr); // Set trace bit and mask interrupts
      subrelease();
      subgo();

      // Wait for TRACE interrupt on sub CPU
      usleep(50);
      while(!(readbyte(0xA1200F) & 0x20));

      subreq();
      subsetbank(0);
      writeword(SUBREG_SR, (readword(SUBREG_SR) & 0x00FF) | (oldsr & 0xFF00));
      showreg(SUBREGADDR);
      d68k_instr(1);
      subrelease();

      continue;
    }

    if(strcmp(token, "subskip") == 0)
    {
      submon();
      subreq();
      d68k_skip_instr(1);
      subrelease();
      subrun();
      continue;
    }

    if(strcmp(token, "boot") == 0)
    {
      parse_word(token, &l);
      boot_img(token);
      continue;
    }

    if(strcmp(token, "bootsp") == 0)
    {
      parse_word(token, &l);
      boot_sp_from_iso(token);
      continue;
    }

    if(strcmp(token, "break") == 0)
    {
      int b;
      if(!*l)
      {
        // List breakpoints
        for(b = 0; b < MAX_BREAKPOINTS; ++b)
        {
          if(breakpoints[b][0] == -1)
          {
            break;
          }
          printf("Breakpoint at $%06X\n", (u32)breakpoints[b][0]);
        }
        continue;
      }
      u32 address = parse_int(&l, 12);
      for(b = 0; b < MAX_BREAKPOINTS; ++b)
      {
        if(breakpoints[b][0] == -1)
        {
          printf("Put a breakpoint at $%06X\n", address);
          breakpoints[b][0] = (int)address;
          breakpoints[b][1] = readword(address);
          breakpoints[b+1][0] = -1;
          break;
        }
      }
      continue;
    }

    if(strcmp(token, "delete") == 0)
    {
      u32 address = parse_int(&l, 12);
      int b;
      for(b = 0; b < MAX_BREAKPOINTS; ++b)
      {
        if(breakpoints[b][0] == -1)
        {
          break;
        }
        while(breakpoints[b][0] == (int)address)
        {
          int nb;
          for(nb = b; nb < MAX_BREAKPOINTS; ++nb)
          {
            if(breakpoints[nb][0] == -1)
            {
              break;
            }
            breakpoints[nb][0] = breakpoints[nb+1][0];
            breakpoints[nb][1] = breakpoints[nb+1][1];
          }
          printf("Deleted a breakpoint at $%06X\n", address);
        }
      }
      continue;
    }

    if(strcmp(token, "subreset") == 0)
    {
      subreset();
      continue;
    }

    if(strcmp(token, "bdpdump") == 0)
    {
      bdpdump = parse_int(&l, 12);
      printf("bdp debug trace %sabled.\n", bdpdump ? "en" : "dis");
      continue;
    }
  } while(0);
  
  signal(SIGINT, SIG_DFL);
  rl_callback_handler_install("bdb > ", on_line_input);
}

