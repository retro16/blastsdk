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

void breakpoint_interrupt(int sig)
{
  (void) sig;

  cleanup_breakpoints();
  signal(SIGINT, goto_mainloop);
  goto_mainloop(sig);
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

void showreg()
{
  u8 regs[17*4 + 2];
  readmem(regs, REGADDR, sizeof(regs));

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

  printf("Requesting sub CPU ...\n");
  subreq();

  printf("Uploading SP (%d bytes) ...\n", spsize);
  subsendmem(0x6000 + SPHEADERSIZE, sp_start, spsize);
  printf("Set sub CPU reset vector to beginning of sub code.\n");
  subwritelong(0x000004, 0x6000 + SPHEADERSIZE);
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
  subwritelong(0x000004, 0x6000 + SPHEADERSIZE);

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

  showreg();
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
    ++*line;
    skipblanks(line);
    return r + **line - '0';
  }
  return -1;
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

int main(int argc, char **argv)
{
  if(argc < 2)
  {
    fprintf(stderr, "Missing parameter.\n");
    usage();
    exit(1);
  }

  open_debugger(argv[1]);

	printf("Blast ! debugger. Connected to %s", argv[1]);

  if(argc >= 3)
  {
    printf("\n");
    boot_img(argv[2]);
    sendcmd(CMD_EXIT, 0);
    readdata();
  }

  char lastline[1024] = "";
  for(;;)
  {
    const char *l;
    char token[1024];

    sigsetjmp(mainloop_jmp, 1);
    signal(SIGINT, SIG_DFL);

    if(!(l = readline("\nbdb > ")))
    {
      break;
    }
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
      return 0;
    }

    if(strcmp(token, "flush") == 0)
    {
      readdata();
      hexdump(inp, inpl, 0);

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
        printf("cmd=%02X\n", cmd);
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
      do {
        readdata();
        if((inp[0] & 0xE0) == 0 && inp[0] & 0x03)
        {
          // The genesis sent data
          fwrite(&inp[1], 1, inp[0] & 0x03, stdout);
        }
      } while((inp[0] & 0xE0) == 0 && inp[0] & 0x03);
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

        showreg();
      }

      continue;
    }

    if(strcmp(token, "ping") == 0)
    {
      printf("Ping");
      fflush(stdout);

      sendcmd(CMD_HANDSHAKE, 0);

      printf(" ... ");
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

      sendcmd(CMD_HANDSHAKE, 0xFFFFFF);

      printf(" ... ");
      fflush(stdout);

      readdata();
      if(inpl != 4 || inp[1] != 0xFF || inp[2] != 0xFF || inp[3] != 0xFE)
      {
        printf("crash !\n");
        hexdump(inp, inpl, 0);
        continue;
      }
      else
      {
        printf("OK !\n");
        hexdump(inp, inpl, 0);
      }

      continue;
    }

		if(strcmp(token, "reg") == 0)
		{
      showreg();

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

    if(strcmp(token, "blsbuild") == 0)
    {
      system("blsbuild");
      continue;
    }

    if(strcmp(token, "d68k") == 0)
    {
      u32 address = parse_int(&l, 12);
      u32 size = parse_int(&l, 12);
      u8 *data = (u8 *)malloc(size);
      readmem(data, address, size);

      char out[262144];
      int suspicious;
      d68k(out, 262144, data, size, address, 1, 1, &suspicious);
      printf("%s\n", out);

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
      continue;
    }

    if(strcmp(token, "step") == 0)
    {
      int oldsr = readword(REG_SR);
      writeword(REG_SR, 0xA700);

      sendcmd(CMD_EXIT, 0);
      readdata();
      readdata();

      writeword(REG_SR, oldsr);
      showreg();
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

    if(strcmp(token, "bdpdump") == 0)
    {
      bdpdump = parse_int(&l, 12);
      printf("bdp debug trace %sabled.\n", bdpdump ? "en" : "dis");
      continue;
    }
  }
    
  printf("\n");

  return 0;
}

