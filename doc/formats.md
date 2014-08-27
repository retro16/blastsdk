Image formats
=============

blsgen can generate many types of output formats, each of them having their own specificities:

Genesis cartridge (gen)
-----------------------

This is a raw genesis cart image, in flat binary format. It is directly mapped by the genesis at addresses 000000-400000.

    000000 : stack pointer
    000004 : reset vector (ROM entry point)
    000008-0000FF : Interrupt vectors
    000100 : 'SEGA' encoded in ASCII
    000104-0001FF : SEGA Genesis header
    000200-3FFFFF : cart data (may be smaller)


Sega CD image (scd)
-------------------

This is a raw Sega CD image (in "iso" mode 1 format, 2048 bytes per block). It can be burnt as the first track of a CD.


Virtual cart (vcart)
--------------------

This special type of image can run in Sega CD program RAM to simulate a cartridge (useful to test programs).

Size is limited to 128k, RAM above FFFD00 must not be used.

To run on a Sega CD, a boot CD with the correct loader is required. When running on the Sega CD, interrupt processing is delayed.

To run on the genesis, simply burn the ROM as-is and it will run.

Header at 0x100 must contain 'SEGA VIRTUALCART'


### Boot loader process

When loaded from the Sega CD, the following procedure must be done :

On the sub CPU :

 * Copy the area 020000-040000 to 020000-040000 in the program RAM.
 * Switch SCD to mode 2 (256k word RAM) and put the header (000000-000200) in word RAM, then switch word RAM to main CPU.
 
On the main CPU :

 * Wait for word RAM.
 * Read the ROM header and set the interrupt vectors in the reserved RAM area (FFFD00).
 * Set program RAM access to bank 1 and enable write protection.
 * Set the stack pointer based on ROM header.
 * Mask interrupts in SR.
 * Reset VDP registers to default values.
 * Reset gamepad registers (Level 2 interrupt may be left enabled for BDA).
 * Jump to the entry point specified in ROM header.


RAM program (ram)
-----------------

This type of program entirely runs in the Genesis main RAM (64k). When blsgen creates this type of image, it adds a correct genesis ROM header and a small boot loader.

Since these programs may be run by the Sega CD, RAM after FFFD00 is reserved.

The image MUST be precisely 0x010100 bytes long.

ROM header is not copied in RAM to make more room.

    000000 : stack pointer
    000004 : reset vector (ROM entry point of boot loader)
    000008-0000FF : Interrupt vectors
    000100 : 'SEGA RAM PROGRAM' encoded in ASCII
    000110-0001FF : SEGA Genesis header
    000200 : RAM entry point
    000204-0003FF : Boot loader
    000400-0100FF : Program to be copied in RAM


### Boot loader process

The provided boot loader will do the following :

 * Unlock TMSS
 * Copy 000400-0100FF to FF0000-FFFCFF
 * Jump at address specified at 000200

The process should be the same when loading from Sega CD or other loaders.
