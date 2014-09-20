#include <stdio.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "bls.h"
#include "blsparse.h"
#include "blsfile.h"
#include "d68k_mod.h"

void help()
{
  fprintf(stderr, "Usage: d68k [-acl] [-o offset] [-s size] [-i instructions] FILE ADDRESS [SYMFILE]\n"
  "Disassembles m68k binary FILE starting at ADDRESS, using optional SYMFILE.\n"
  "    -a assemble FILE before disassembling\n"
  "    -c shows cycles for each instruction.\n"
  "    -l shows labels.\n"
  "    -o offset in file.\n"
  "    -s max number of bytes to dissasemble.\n"
  "    -i max number of instructions to dissasemble.\n"
  );
  exit(1);
}


int main(int argc, char **argv)
{
  int cycles = 0;
  int labels = 0;
  int assemble = 0;
  int instructions = -1;
  u32 size = 0xFFFFFFF;
  u32 offset = 0;

  int c;
  while((c = getopt (argc, argv, "aclo:s:i:")) != -1)
  {
    switch(c)
    {
      case 'c':
      cycles = 1;
      break;

      case 'l':
      labels = 1;
      break;

      case 'a':
      assemble = 1;
      break;

      case 'o':
      offset = parse_int(optarg);
      break;

      case 's':
      size = parse_int(optarg);
      break;

      case 'i':
      instructions = parse_int(optarg);
      break;

      default:
      help();
      break;
    }
  }

  if(argc < optind + 2 || argc > optind + 3)
  {
    help();
  }

  if(argc == optind + 3)
  {
/*    if(!parsedsymfile(argv[fi + 2]))
    {
      fprintf(stderr, "Could not parse symbol file %s.\n", argv[fi + 2]);
      return 1;
    }*/
    labels = 1;
  }

  u32 address = parse_int(argv[optind + 1]);

  char infilename[4096];
  strcpy(infilename, argv[optind + 0]);

  if(assemble)
  {
    snprintf(infilename, 4096, "%s.tmp", argv[optind + 0]);
    char cmdline[4096];
    snprintf(cmdline, 4096, "asmx -b 0x%08X -o %s -C 68000 %s > /dev/null 2>/dev/null", address, infilename, argv[optind + 0]);
    system(cmdline);
  }

  u8 *data;
  u32 realsize = readfile(infilename, &data);
  if(offset >= realsize)
  {
    fprintf(stderr, "Offset out of bounds.\n");
    exit(1);
  }
  if(offset + size > realsize)
  {
    size = realsize - offset;
  }

  if(assemble)
  {
    unlink(infilename);
  }

  char *dasm = malloc(size * 40);

  int suspicious;
  int64_t r = d68k(dasm, size * 40, data + offset, size, instructions, address, labels, cycles, &suspicious);
  free(data);

  if(r < 0)
  {
    free(dasm);
    fprintf(stderr, "d68k error : %s\n", d68k_error(r));
    return 2;
  }

  printf("%s", dasm);
  free(dasm);

  return 0;
}
