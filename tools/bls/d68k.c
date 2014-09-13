#include <stdio.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "bls.h"

void help()
{
  fprintf(stderr, "Usage: d68k [-acl] FILE ADDRESS [SYMFILE]\nDisassembles m68k binary FILE starting at ADDRESS, using optional SYMFILE.\n    -a assemble FILE before disassembling\n    -c shows cycles for each instruction.\n    -l shows labels.\n");
  exit(1);
}

int main(int argc, char **argv)
{
  int cycles = 0;
  int labels = 0;
  int assemble = 0;

  int c;  
  while((c = getopt (argc, argv, "acl")) != -1)
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

  const char *addrptr = argv[optind + 1];
  u32 address = parse_int(&addrptr, 10);

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
  int size = readfile(infilename, &data);

  if(assemble)
  {
    unlink(infilename);
  }

  char *dasm = malloc(size * 40);

  int suspicious;
  int r = d68k(dasm, size * 40, data, size, address, labels, cycles, &suspicious);

  printf("%s", dasm);
  free(data);
  free(dasm);

  if(r)
  {
    fprintf(stderr, "d68k error : %s\n", d68k_error(r));
    return 2;
  }

  return 0;
}
