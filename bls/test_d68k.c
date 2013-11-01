#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "bls.h"

int main(int argc, char **argv)
{
  if(argc != 2)
  {
    printf("Missing asm file\n");
    return 1;
  }

  char cmdline[4096];
  snprintf(cmdline, 4096, "asmx -C 68k -b 0xFF0000 -l test1.lst -o test1.bin '%s' &>/dev/null", argv[1]);
  if(system(cmdline) & 127)
  {
    printf("Could not assemble file %s.\n", argv[1]);
    return 1;
  }

  u8 *test1;
  int test1len = readfile("test1.bin", &test1);

  char *dasm = malloc(test1len * 40);
  int suspicious;
  int r = d68k(dasm, test1len * 40, test1, test1len, 0xFF0000, 1, &suspicious);

  if(r)
  {
    printf("Error while disassembling : %s.\n", d68k_error(r));
  }

  writefile("test2.asm", (const u8*)dasm, strlen(dasm));

  snprintf(cmdline, 4096, "asmx -e -w -C 68k -b 0xFF0000 -l test2.lst -o test2.bin test2.asm");
  if(system(cmdline) & 127)
  {
    printf("Re-assembly failed.\n");
    return 2;
  }

  u8 *test2;
  int test2len = readfile("test2.bin", &test2);

  if(test1len != test2len)
  {
    printf("Sizes don't match : %d -> %d.\n", test1len, test2len);
    return 2;
  }

  if(memcmp(test1, test2, test1len))
  {
    printf("Content does not match.\n");
    return 2;
  }

  printf("%s\n; Disassembly successful\n;   binary length: %d\n;   suspicious opcodes : %d\n", dasm, test1len, suspicious);
}
