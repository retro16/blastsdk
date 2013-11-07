#include <stdio.h>
#include "bls.h"

int main(int argc, char **argv)
{
  if(argc != 3 && argc != 4)
  {
    fprintf(stderr, "Usage: d68k FILE ADDRESS [SYMFILE]\nDisassembles m68k binary FILE starting at ADDRESS, using optional SYMFILE.\n");
    return 1;
  }

  int labels = 0;
  if(argc == 4)
  {
/*    if(!parsedsymfile(argv[3]))
    {
      fprintf(stderr, "Could not parse symbol file %s.\n", argv[3]);
      return 1;
    }*/
    labels = 1;
  }

  u8 *data;
  int size = readfile(argv[1], &data);
  const char *addrptr = argv[2];
  u32 address = parse_int(&addrptr, 10);

  char *dasm = malloc(size * 40);

  int suspicious;
  int r = d68k(dasm, size * 40, data, size, address, labels, &suspicious);

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
