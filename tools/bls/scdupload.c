#include <stdio.h>
#include "bls.h"

void usage()
{
  printf("Usage: scdupload DEVICE IP SP\nUploads IP and SP binaries through DEVICE to the SCD and run them.\n");
  exit(1);
}

int main(int argc, char **argv)
{
  if(argc != 4)
  {
    usage();
  }

  open_debugger(argv[1]);
 
  sendfile(argv[2], 0xF000);
  subsendfile(argv[3], 0x6000 + SPHEADERSIZE);

  // Set stack pointer to a safe value
  writelong(REG_SP, 0xFFF800);

  // Set PC to the boot address
  writelong(REG_PC, 0xFF0000 + SECCODESIZE);

  // Exit debugger
  sendcmd(CMD_EXIT, 0);

	return 0;
}
