#include <stdio.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "bls.h"

int main(int argc, char **argv)
{
  int cycles = 0;
  int labels = 0;

  int c;  
  while((c = getopt (argc, argv, "cl")) != -1)
  {
    switch(c)
    {
      case 'c':
      cycles = 1;
      break;
      
      case 'l':
      labels = 1;
      break;
      
      default:
      fprintf(stderr, "Usage: d68k [-cl] FILE ADDRESS [SYMFILE]\nDisassembles m68k binary FILE starting at ADDRESS, using optional SYMFILE.\n   -c shows cycles for each instruction.\n   -l shows labels.\n");
      return 1;
    }
  }
  
  if(argc < optind + 2 || argc > optind + 3)
  {
    fprintf(stderr, "Usage: d68k [-cl] FILE ADDRESS [SYMFILE]\nDisassembles m68k binary FILE starting at ADDRESS, using optional SYMFILE.\n   -c shows cycles for each instruction.\n   -l shows labels.\n");
    return 1;
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

  u8 *data;
  int size = readfile(argv[optind + 0], &data);
  const char *addrptr = argv[optind + 1];
  u32 address = parse_int(&addrptr, 10);

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
