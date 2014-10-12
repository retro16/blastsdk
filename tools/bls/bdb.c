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
#include "blsparse.h"
#include "blsbinparse.h"
#include "blsfile.h"
#include "bdp.h"
#include "d68k_mod.h"
#include "blsload_sim.bin.c"

////// Globals

sigjmp_buf mainloop_jmp;
#define MAX_BREAKPOINTS 1024

int cpu; // Current CPU
char regname[][3] = { "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "pc", "sr"};

int use_cdsim = 1; // Use CD-ROM simulator
FILE *cdsim_file = 0; // CD simulator file

static void usage();
static void goto_mainloop(int sig);
static void erase_prompt();
static void restore_prompt();

static void usage()
{
  fprintf(stderr, "Usage: bdb [-c] DEVICE [IMAGE]\n"
          "The Blast ! debugger.\n"
          "Communicates to a SEGA Mega Drive / Genesis via an arduino.\n"
          "\n"
          "Options:"
          "\n"
          "      -c       use real CD-ROM drive instead of the simulator (does not work/unimplemented)\n"
          "      DEVICE   either a serial device (/dev/xxx) or an IP address.\n"
          "      IMAGE    boot this image at bdb startup\n"
         );
}

static void goto_mainloop(int sig)
{
  (void)sig;
  printf("SIGINT\n");
  rl_free_line_state();
  rl_reset_after_signal();

  // Go back to main loop
  siglongjmp(mainloop_jmp, 0);
}


static void on_bdp_comm(const u8 *data, int len)
{
  if(len == 3 && data[0] < 0x07) {
    printf("CD-ROM seek at sector %02X%02X%02X\n", (u32)data[0], (u32)data[1], (u32)data[2]);
    // ROMREAD request from simulated BIOS
    u32 offset = getint(data, 3) * CDBLOCKSIZE;
    if(cdsim_file) {
      fseek(cdsim_file, offset, SEEK_SET);
    } else {
      printf("BDP requested CD read at %08X but no CD image is loaded\n", offset);
    }
  } else if(len == 3 && data[0] == 0x07) {
    printf("CD-ROM read sector. Upload to buffer at %02X%02X\n", (u32)data[0], (u32)data[1], (u32)data[2]);
    // CDCREAD request from simulated BIOS
    if(cdsim_file) {
      u8 buf[CDBLOCKSIZE + 4];
      u32 offset = ftell(cdsim_file);
      fread(buf, 1, CDBLOCKSIZE, cdsim_file);
      buf[CDBLOCKSIZE] = (offset / CDBLOCKSIZE / 75 / 60); // mm
      buf[CDBLOCKSIZE] = (offset / CDBLOCKSIZE / 75) % 60; // ss
      buf[CDBLOCKSIZE] = (offset / CDBLOCKSIZE) % (75 * 60); // ff
      buf[CDBLOCKSIZE] = 1; // md (0 = audio, 1 = mode 1, 2 = mode 2)

      writemem(1, getint(&data[1], 2), buf, sizeof(buf));
    }
  } else if(data[0] >= 0x20 || data[0] == '\r' || data[0] == '\n' || data[0] == '\t') {
    printf("%.*s", len, data);
  } else {
    hexdump(data, len, 0);
  }
}


void on_unknown(const u8 inp[36], int inpl)
{
  erase_prompt();
  if(inpl == 4 && inp[0] >= 1 && inp[0] <= 3) {
    // BDP communication request
    on_bdp_comm(&inp[1], inp[0]);
  } else {
    printf("Received unknown data from the genesis\n");
    hexdump(inp, inpl, 0);
    exit(1);
  }
  restore_prompt();
}

void on_breakpoint(int cpu, u32 address)
{
  erase_prompt();
  printf("%s CPU: breakpoint reached at %06X\n", cpu ? "Sub" : "Main", address & 0xFFFFFF);
  restore_prompt();
}

void on_exception(int cpu, int ex)
{
  erase_prompt();

  if(ex >= 0x20) {
    printf("%s CPU: TRAP #%02d\n", cpu ? "Sub" : "Main", ex - 0x20);
  } else {
    printf("%s CPU: Exception %02d triggered\n", cpu ? "Sub" : "Main", ex);
  }

  restore_prompt();
}

static void erase_prompt()
{
  char *lb = rl_line_buffer;

  while(*(lb++)) {
    printf("\b \b\b");  // Erase readline line
  }

  printf("\b\b\b\b\b\b\b       \b\b\b\b\b\b\b"); // Erase "bdb > " prompt
}

static void restore_prompt()
{
  printf("bdb %c> %s", (cpu ? 'S' : 'M') + (is_running(cpu) ? 0 : 0x20), rl_line_buffer); // Restore readline line display
  fflush(stdout);
}

void parse_word(char *token, const char **line)
{
  skipblanks(line);

  while(**line > ' ') {
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
    ||(**line >= '0' && **line <= '9')) {
    *token = **line;
    ++token;
    ++*line;
  }

  *token = '\0';
  skipblanks(line);
}

int parse_register(const char **line)
{
  skipblanks(line);
  int r = -1;

  if(**line == 's' || **line == 'S') {
    ++*line;

    if(**line == 'r' || **line == 'R') {
      ++*line;
      return 17;
    }
  } else if(**line == 'p' || **line == 'P') {
    ++*line;

    if(**line == 'c' || **line == 'C') {
      ++*line;
      return 16;
    }
  } else if(**line == 'd' || **line == 'D') {
    r = 0;
  } else if(**line == 'a' || **line == 'A') {
    r = 8;
  }

  if(r == -1) {
    return -1;
  }

  ++*line;

  if(**line >= '0' && **line <= '7') {
    r += **line - '0';
    ++*line;
    skipblanks(line);
    return r;
  }

  return -1;
}

u32 parse_address_skip(const char **line)
{
  skipblanks(line);
  if(**line == '#') {
    // Ignore # since it has no meaning
    ++(*line);
    skipblanks(line);
  }
  if((**line >= '0' && **line <= '9') || **line == '~' || **line == '-' || **line == '$') {
    return parse_int_skip(line);
  }

  if(**line == '(' && (*line)[1] && (*line)[2] && (*line)[3] == ')') {
    // Register
    ++(*line); // Skip opening parenthese
    int reg = parse_register(line);
    ++(*line); // Skip closing parenthese
    return readreg(cpu, reg);
  }

  char symname[4096];
  parse_word(symname, line);
  sv symval = getdsymval(symname);

  if(symval == -1) {
    printf("Unknown symbol %s\n", symname);
    return 0;
  }

  return (u32)symval;
}

void showreg(int cpu)
{
  u32 regs[18];
  readregs(cpu, regs);

  printf("%s cpu registers :\n", cpu ? "sub" : "main");

  int r;
  int t;

  for(t = 0; t <= 8; t += 8) {
    for(r = 0; r < 8; ++r) {
      printf("%s:%08X ", regname[r + t], regs[r + t]);
    }

    printf("\n");
  }

  printf("pc:%08X sr:%04X [ %c %c %d %c %c %c %c %c ]\n",
      regs[REG_PC],
      regs[REG_SR],
      (regs[REG_SR] & 0x8000) ? 'T' : ' ',
      (regs[REG_SR] & 0x2000) ? 'S' : 'U',
      (regs[REG_SR] >> 8) & 0x7,
      (regs[REG_SR] & 0x0010) ? 'X' : ' ',
      (regs[REG_SR] & 0x0008) ? 'N' : ' ',
      (regs[REG_SR] & 0x0004) ? 'Z' : ' ',
      (regs[REG_SR] & 0x0002) ? 'V' : ' ',
      (regs[REG_SR] & 0x0001) ? 'C' : ' '
  );
}

void boot_cd(u8 *image, int imgsize)
{
  maintarget = target_scd2;

  const u8 *ip_start;
  int ipsize;
  ipsize = getipoffset(image, &ip_start);

  const u8 *sp_start;
  int spsize;
  spsize = getspoffset(image, &sp_start);

  u32 seccodesize = detect_region(image);

  printf("CD-ROM image. IP=%04X-%04X (%d bytes). SP=%04X-%04X (%d bytes).\n", (u32)(ip_start-image), (u32)(ip_start - image + ipsize - 1), ipsize, (u32)(sp_start - image), (u32)(sp_start - image + spsize - 1), spsize);

  printf("Entering monitor mode on main CPU\n");
  stopcpu(0);

  printf("Freezing sub CPU\n");
  subfreeze();

  printf("Clearing communication flags ...\n");
  u8 clear[0x10];
  memset(clear, '\0', sizeof(clear));
  writemem(0, 0xA12010, clear, sizeof(clear)); // Clear other comm words

  printf("Clearing BDP buffer ...\n");
  writelong(1, 0x000030, 0);

  printf("Uploading IP (%d bytes) ...\n", ipsize);
  writemem(0, 0xFF0000 + seccodesize, ip_start, ipsize);
  printf("Setting main CPU registers.\n");
  setreg(0, REG_PC, 0xFFFF0000 + seccodesize);
  setreg(0, REG_SP, 0xFFFFD000);
  setreg(0, REG_SR, 0x2700);

  printf("Uploading SP (%d bytes) ...\n", spsize);
  writemem_verify(1, 0x6000, sp_start, spsize);

  if(use_cdsim) {
    printf("Uploading simulated BIOS ...\n");
    writemem(1, 0x200, blsload_sim_bin, sizeof(blsload_sim_bin));

    update_bda_sub(image, imgsize);

    printf("Setting sub CPU registers.\n");
    setreg(1, REG_PC, 0x00000200);
    setreg(1, REG_SP, readlong(1, 0));
    setreg(1, REG_SR, 0x2700);
  } else {
    printf("Boot on real BIOS is still unimplemented, sorry !\n");
    exit(2);
    // TODO
    // How do you make the real BIOS reload the SP header ?
  }

  printf("Ready to boot.\n");
}

void boot_sp(u8 *image, int imgsize)
{
  maintarget = target_scd2;

  const u8 *sp_start;
  int spsize;
  spsize = getspoffset(image, &sp_start);

  printf("CD-ROM image. SP=%04X-%04X (%d bytes).\n", (u32)(sp_start - image), (u32)(sp_start - image + spsize - 1), spsize);

  printf("Freezing sub CPU\n");
  subfreeze();

  printf("Uploading SP (%d bytes) ...\n", spsize);
  writemem(1, 0x6000, sp_start, spsize);

  if(use_cdsim) {
    printf("Uploading simulated BIOS ...\n");
    writemem(1, 0x200, blsload_sim_bin, sizeof(blsload_sim_bin));

    update_bda_sub(image, imgsize);

    printf("Setting sub CPU registers.\n");
    setreg(1, REG_PC, 0x00000200);
    setreg(1, REG_SP, readlong(1, 0));
    setreg(1, REG_SR, 0x2700);
    printf("New SP program loaded.\n");
  } else {
    printf("Boot on real BIOS is still unimplemented, sorry !\n");
    exit(2);
    // TODO
    // How do you make the real BIOS reload the SP header ?
  }
}

// v is the vector offset (0x08-0xBC)
// value is the target of the vector
void gen_setvector(u32 v, u32 value)
{
  if(v == 0x70) {
    // LEVEL4/HBLANK interrupt
    char romid[17];
    romid[16] = '\0';
    readmem(0, (u8 *)romid, 0x120, 16);

    if(strstr(romid, "BOOT ROM")) {
      printf("Sega CD detected : write the HBLANK vector at the special address\n");

      if((value & 0x00FF0000) != 0x00FF0000) {
        printf("Warning : cannot set HBLANK outside main RAM for Sega CD.\n");
      }

      writelong(0, 0xA12006, value & 0xFFFF);
      return;
    }
  }

  // Fetch wrapper from genesis
  u32 wrapper = readlong(0, v) & 0x00FFFFFF;

  if(wrapper < 0x200000) {
    // Not in RAM : cannot change vector
    printf("Warning : cannot set vector %02X : target not in RAM.\n", v);
    return;
  }

  printf("Set interrupt vector code %02X at %06X, jumping to %06X\n", v, wrapper, value);
  writeword(0, wrapper, 0x4EF9); // Write jmp instruction
  writelong(0, wrapper + 2, value); // Write target address
}

void boot_ram(u8 *image, int imgsize)
{
  if(imgsize != 0xFF00) {
    printf("Incorrect image size (must be 0xFF00)\n");
    return;
  }

  maintarget = target_ram;

  stopcpu(0);

  u32 sp = getint(image, 4);
  u32 pc = getint(image + 0xFC, 4);

  u32 v;

  for(v = 0x08; v < 0xC0; v += 4) {
    u32 value = getint(image + v, 4);

    if(value != pc && value >= 0x200) {
      if(v == 0x68) {
        printf("Warning : Image overrides level 2 (BDA/pad) interrupt\n");
      }

      gen_setvector(v, value);
    }
  }

  // Upload RAM program
  writemem(0, 0xFF0000, image + 0x200, imgsize - 0x200);

  // Set SR, SP and PC
  setreg(0, REG_SP, sp);
  setreg(0, REG_PC, pc);
  setreg(0, REG_SR, 0x2700);

  printf("Ready to boot.\n");
}

void boot_img(const char *filename)
{
  u8 *image;
  int imgsize = readfile(filename, &image);

  if(imgsize < 0x200) {
    printf("Image too small.\n");
    free(image);
    return;
  }

  switch(getimgtype(image, imgsize)) {
  case 0:
    printf("Invalid image type.\n");
    break;

  case 1:
    printf("Cannot boot ROM cartridges.\n");
    break;

  case 2:
    boot_cd(image, imgsize);
    if(cdsim_file) fclose(cdsim_file);
    cdsim_file = fopen(filename, "rb");
    break;

  case 4:
    boot_ram(image, imgsize);
    break;
  }

  free(image);

  cpu = 0;
  d68k_setbus(bus_main);
  d68k_readsymbols(filename);

  showreg(0);
}

void boot_sp_from_iso(const char *filename)
{
  u8 *image;
  int imgsize = readfile(filename, &image);

  if(imgsize < 0x200) {
    printf("Image too small.\n");
    free(image);
    return;
  }

  switch(getimgtype(image, imgsize)) {
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

// Assemble a file and send it to a memory buffer
u32 asmfile(const char *filename, u8 *target, u32 org)
{
  char cmdline[4096];
  snprintf(cmdline, 4096, "asmx -w -e -b 0x%08X -l /dev/stderr -o /dev/stdout -C 68000 %s", org, filename);
  printf("%s\n", cmdline);
  FILE *a = popen(cmdline, "r");
  u32 codesize = 0;

  while(!feof(a)) {
    u32 readsize = fread(target, 1, 4096, a);
    codesize += readsize;
    target += readsize;
  }

  if(pclose(a)) {
    printf("Warning : asmx returned an error\n");
  }

  return codesize;
}

void d68k_instr(int cpu)
{
  u32 address = readreg(cpu, REG_PC);
  u8 data[10];
  readmem(cpu, data, address, 10);

  char out[256];
  int suspicious;
  d68k(out, 256, data, 10, 1, address, 1, 1, 1, &suspicious);
  printf("(next) %s", out);
}

void d68k_skip_instr(int cpu)
{
  u32 address = readreg(cpu, REG_PC);
  u8 data[10];
  readmem(cpu, data, address, 10);

  char out[256];
  int suspicious;
  address = d68k(out, 256, data, 10, 1, address, 0, 0, 0, &suspicious);
  printf("(skip) %s", out);
  setreg(cpu, REG_PC, address);
  d68k_instr(cpu);
}

void d68k_next_instr(int cpu)
{
  u32 address = readreg(cpu, REG_PC);
  u8 data[10];
  readmem(cpu, data, address, 10);

  char out[256];
  int suspicious;
  u32 targetaddress = d68k(out, 256, data, 10, 1, address, 0, 0, 0, &suspicious);

  int hasbp = has_breakpoint(cpu, targetaddress);
  if(!hasbp) set_breakpoint(cpu, targetaddress);
  reach_breakpoint(cpu);
  if(!hasbp) delete_breakpoint(cpu, targetaddress);

  showreg(cpu);
  d68k_instr(cpu);
}


char lastline[1024] = "";
void on_line_input(char *l);

int main(int argc, char **argv)
{
  int c;

  maintarget = target_gen;

  while((c = getopt(argc, argv, "hc")) != -1) {
    switch(c) {
      case 'h':
        usage();
        return 0;

      case 'c':
        use_cdsim = 0;
        break;
    }
  }

  if(argc - optind < 1) {
    fprintf(stderr, "Missing parameter.\n");
    usage();
    exit(1);
  }

  open_debugger(argv[optind], on_unknown, on_breakpoint, on_exception);

  printf("Blast ! debugger. Connected to %s\n", argv[1]);

  cpu = 0;
  d68k_setbus(bus_main);

  if(argc - optind >= 2) {
    printf("\n");
    boot_img(argv[optind + 1]);
  } else {
    d68k_readsymbols(NULL);
  }

  sigsetjmp(mainloop_jmp, 1);
  signal(SIGINT, SIG_DFL);

  char prompt[16];
  snprintf(prompt, 16, "bdb %c> ", (cpu ? 'S' : 'M') + (is_running(cpu) ? 0 : 0x20));
  rl_callback_handler_install(prompt, on_line_input);

  for(;;) {
    fd_set readset;
    int selectresult;

    bdp_readbuffer();

    FD_ZERO(&readset);
    FD_SET(0, &readset);
    FD_SET(genfd, &readset);

    struct timeval select_timeout;
    select_timeout.tv_sec = 0;
    select_timeout.tv_usec = 40000;
    selectresult = select(genfd + 1, &readset, NULL, NULL, &select_timeout);

    if(selectresult > 0) {
      if(FD_ISSET(0, &readset)) {
        rl_callback_read_char();
      } else if(FD_ISSET(genfd, &readset)) {
        bdp_readdata();
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
    if(*l) {
      strcpy(lastline, l);
      add_history(l);
    } else {
      // Replay last line
      if(!lastline[0]) {
        continue;
      }

      l = lastline;
    }

    signal(SIGINT, goto_mainloop); // Handle Ctrl-C : jump back to prompt

    if(strncmp(l, "main", 4) == 0) {
      if(cpu != 0) {
        printf("Switch to main cpu\n");
      }

      cpu = 0;
      l += 4;

      d68k_setbus(bus_main);
    }

    if(strncmp(l, "sub", 3) == 0) {
      if(cpu != 1) {
        printf("Switch to sub cpu\n");
      }

      cpu = 1;
      l += 3;

      d68k_setbus(bus_sub);
    }

    parse_token(token, &l);

    if(strcmp(token, "q") == 0) {
      exit(0);
    }

    if(strcmp(token, "cpu") == 0) {
      printf("Working on %s cpu", cpu ? "sub" : "main");
      continue;
    }

    if(strcmp(token, "asmx") == 0) {
      parse_word(token, &l);
      u32 address = parse_address_skip(&l);

      if(!address) {
        address = cpu ? 0x006000 : 0xFF0000;
      }

      u8 obj[65536];
      u32 osize = asmfile(token, obj, address);

      writemem(cpu, address, obj, osize);
      setreg(cpu, REG_PC, address);
      showreg(cpu);

      continue;
    }

    if(strcmp(token, "r") == 0) {
      parse_token(token, &l);
      u32 address = parse_address_skip(&l);
      u32 value;

      switch(token[0]) {
      case 'b':
        value = readbyte(cpu, address);
        break;

      case 'w':
        value = readword(cpu, address);
        break;

      case 'l':
        value = readlong(cpu, address);
        break;
      }

      printf("%06X: %08X\n", address, value);
      continue;
    }

    if(strcmp(token, "w") == 0) {
      parse_token(token, &l);
      u32 address = parse_address_skip(&l);
      u32 value = parse_address_skip(&l);

      switch(token[0]) {
      case 'b':
        writebyte(cpu, address, value);
        break;

      case 'w':
        writeword(cpu, address, value);
        break;

      case 'l':
        writelong(cpu, address, value);
        break;
      }

      printf("%06X: %08X\n", address, value);
      continue;
    }

    if(strcmp(token, "sendfile") == 0) {
      u32 address = parse_address_skip(&l);
      parse_word(token, &l);
      sendfile(cpu, token, address);
      continue;
    }

    if(strcmp(token, "go") == 0) {
      startcpu(cpu);
      continue;
    }

    if(strcmp(token, "stop") == 0) {
      stopcpu(cpu);
      continue;
    }

    if(strcmp(token, "bridge") == 0) {
      u8 inp[3];

      if(bridgequery(inp)) {
        printf("OK !\n\n Protocol v%c\n Implementation v%c.%c\n", inp[0], inp[1], inp[2]);
      }
    }

    if(strcmp(token, "reg") == 0) {
      showreg(cpu);
      continue;
    }

    if(strcmp(token, "dump") == 0) {
      u32 address = parse_address_skip(&l);
      skipblanks(&l);
      u32 size = parse_address_skip(&l);
      skipblanks(&l);
      u8 *data = (u8 *)malloc(size);
      readmem(cpu, data, address, size);

      skipblanks(&l);

      if(*l) {
        FILE *f = fopen(l, "w");

        if(!f) {
          printf("Cannot open file %s.\n", l);
          continue;
        }

        fwrite(data, 1, size, f);
        fclose(f);
      } else {
        hexdump(data, size, address);
      }

      continue;
    }

    if(strcmp(token, "showchr") == 0) {
      u32 address = parse_address_skip(&l) * 32;
      u8 data[32];
      readvram(data, address, 32);
      int c, x, y;
      c = 0;
      printf(" --------\n");

      for(y = 0; y < 8; ++y) {
        printf("|");

        for(x = 0; x < 4; ++x) {
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

    if(strcmp(token, "vdump") == 0) {
      u32 address = parse_address_skip(&l);
      skipblanks(&l);
      u32 size = parse_address_skip(&l);
      skipblanks(&l);
      u8 *data = (u8 *)malloc(size);
      readvram(data, address, size);

      skipblanks(&l);

      if(*l) {
        FILE *f = fopen(l, "w");

        if(!f) {
          printf("Cannot open file %s.\n", l);
          continue;
        }

        fwrite(data, 1, size, f);
        fclose(f);
      } else {
        hexdump(data, size, address);
      }

      continue;
    }

    if(strcmp(token, "blsgen") == 0) {
      system("blsgen");
      continue;
    }

    if(strcmp(token, "d68k") == 0) {
      u32 address = parse_address_skip(&l);
      u32 size = parse_address_skip(&l);
      int instructions = -1;

      if(!address) {
        address = readreg(cpu, REG_PC);
        size = 0;
        printf("Starting at PC (%08X)\n", address);
      }

      if(!size) {
        size = 10;
        instructions = 1;
      }

      u8 *data = (u8 *)malloc(size);
      readmem(cpu, data, address, size);

      char out[262144];
      int suspicious;
      d68k(out, 262144, data, size, instructions, address, 1, 1, 1, &suspicious);
      printf("%s\n", out);

      continue;
    }

    if(strcmp(token, "set") == 0) {
      int reg = parse_register(&l);

      if(reg == -1) {
        printf("Unknown register.\n");
        continue;
      }

      u32 value = parse_address_skip(&l);
      setreg(cpu, reg, value);
      showreg(cpu);
      continue;
    }

    if(strcmp(token, "step") == 0 || strcmp(token, "s") == 0) {
      stepcpu(cpu);
      showreg(cpu);
      d68k_instr(cpu);
      continue;
    }

    if(strcmp(token, "next") == 0 || strcmp(token, "n") == 0) {
      d68k_next_instr(cpu);
      continue;
    }

    if(strcmp(token, "skip") == 0) {
      d68k_skip_instr(cpu);
      continue;
    }

    if(strcmp(token, "boot") == 0) {
      parse_word(token, &l);
      boot_img(token);
      continue;
    }

    if(strcmp(token, "bootsp") == 0) {
      parse_word(token, &l);
      boot_sp_from_iso(token);
      continue;
    }

    if(strcmp(token, "break") == 0) {
      if(!*l) {
        list_breakpoints(cpu);
        continue;
      }

      u32 address = parse_address_skip(&l);
      set_breakpoint(cpu, address);
      continue;
    }

    if(strcmp(token, "delete") == 0) {
      u32 address = parse_address_skip(&l);

      if(delete_breakpoint(cpu, address)) {
        printf("Deleted a breakpoint at $%06X\n", address);
      }

      continue;
    }

    if(strcmp(token, "reset") == 0) {
      resetcpu(cpu);
      continue;
    }

    if(strcmp(token, "bdpdump") == 0) {
      int bdpdump = parse_int_skip(&l);
      printf("bdp debug trace %sabled.\n", bdpdump ? "en" : "dis");
      bdp_set_dump(bdpdump);
      continue;
    }

    if(strcmp(token, "sym") == 0) {
      u32 addr = parse_int_skip(&l);
      const char *sym = getdsymat(addr);
      if(sym) {
        printf("$%06X: %s\n", addr & 0xFFFFFF, sym);
      } else {
        printf("$%06X: (no symbol)\n", addr & 0xFFFFFF);
      }
      continue;
    }

    if(strcmp(token, "p") == 0 || strcmp(token, "print") == 0) {
      u32 addr = parse_address_skip(&l);
      printf("$%08X  u32=%u i32=%d", addr, addr, (int)addr);
      int val = addr;
      if(val >= 0x8000 && val < 0x10000) {
        signext(&val, 16);
        printf(" i16=%d", val);
      } else if(val >= 0x80 && val < 0x100) {
        signext(&val, 8);
        printf(" i8=%d", val);
      }
      printf("\n");
      continue;
    }

    if(strcmp(token, "symload") == 0) {
      parse_word(token, &l);
      d68k_readsymbols(token);
      continue;
    }

    if(strcmp(token, "symclear") == 0) {
      parse_word(token, &l);
      d68k_freesymbols();
      printf("Symbol table cleared.\n");
      continue;
    }

    if(strcmp(token, "dbgstatus") == 0) {
      dbgstatus();
      continue;
    }

    if(strcmp(token, "cdsim") == 0) {
      parse_word(token, &l);

      if(cdsim_file) fclose(cdsim_file);
      cdsim_file = fopen(token, "rb");

      if(cdsim_file) {
        printf("Using %s to simulate CD-ROM\n", token);
      } else {
        printf("Could not open image %s\n", token);
      }
      continue;
    }

    if(strcmp(token, "target") == 0) {
      parse_word(token, &l);
      maintarget = target_parse(token);
      printf("Target set to %s\n", target_names[maintarget]);
    }

  } while(0);

  signal(SIGINT, SIG_DFL);
  char prompt[16];
  snprintf(prompt, 16, "bdb %c> ", (cpu ? 'S' : 'M') + (is_running(cpu) ? 0 : 0x20));
  rl_callback_handler_install(prompt, on_line_input);
}


// vim: ts=2 sw=2 sts=2 et
