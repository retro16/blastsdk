Sub CPU idle program

The sub idle program provides a SP that does nothing but service binary loading
routines, which facilitates porting programs from cart to CD-ROM.

To use, include this file in the output section of the blsgen.md file and
include `sub_init.inc` or `sub_init.h` in your main program file.

You must call the `sub_idle_init` function at the beginning of your main
program. You must also call the `sub_idle_vsync` function at each vblank
interrupt and vblank interrupts must be enabled during binary loading.

---------------------------------------

 - binaries sub_idle

Binary sub_idle
---------------

 - provides sub_idle.asm
 - bus sub
 - include blsload_sub.md bdp_sub.md

