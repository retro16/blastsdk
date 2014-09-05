Image formats
=============

blsgen can generate many types of output formats, each of them having their own specificities:

Genesis cartridge (gen)
-----------------------

This is a raw genesis cart image, in flat binary format. It is directly mapped by the genesis at addresses 000000-3FFFFF.

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

This special type of image can simulate a genesis cartridge using Sega CD program RAM (useful to test programs).

ROM size is limited to 128k, RAM above FFFD00 must not be used.

To run on a Sega CD, a boot CD with the correct loader is required. When running on the Sega CD, interrupt processing is delayed.

To run on the genesis, simply burn the ROM as-is and it will run. ROM data starts at 020000 and data between 000200 and 01FFFF is ignored.

Header at 0x100 must contain 'SEGA VIRTUALCART'

The image MUST be exactly 0x040000 bytes.


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
 * Reset VDP registers to default values.
 * Reset gamepad registers (Level 2 interrupt may be left enabled for BDA).
 * Jump to the entry point specified in ROM header.


RAM program (ram)
-----------------

This type of program entirely runs in the Genesis main RAM (64k). When blsgen creates this type of image, it adds a correct genesis ROM header and a small boot loader located in the unused interrupt vectors.

Since these programs may be run by the Sega CD, RAM after FFFD00 is reserved.

The image MUST be precisely 0x00FF00 bytes long.

ROM header is not copied in RAM to make more room.

    000000 : stack pointer
    000004 : reset vector (ROM entry point of boot loader)
    000008-0000BF : Interrupt vectors
    0000C0-0000FB : RAM program boot loader
    0000FC-0000FF : Entry point
    000100 : 'SEGA RAM PROGRAM' encoded in ASCII
    000110-0001FF : SEGA Genesis header
    000200-00FEFF : Program to be copied in RAM


### Boot loader process

The provided boot loader will do the following :

 * Unlock TMSS
 * Copy 000200-00FEFF to FF0000-FFFCFF
 * Jump at address specified at 0000EA

The process should be the same when loading from Sega CD or other loaders.


Compression formats
===================

lz68k (not finalized)
-----

lz68k is a compression format inspired by LZ4, but modified for easier decompression on a m68k CPU. Input and output are byte streams.

### lz68k format :

  number: litteral count
 n bytes: litterals
  number: backcopy offset
  number: backcopy count

Number format is variable size : values <= 127 are on one byte, values between 128 and 32767 are on a word, with the most significant bit ignored (since it is always set)

Stream ends when backcopy offset is 0.


bits field
   5 backcopy length (if > 30, all bits set and size is specified in 1 or 2 extra bytes)
   3 litteral length (if > 6, 3 bits to 1 and followed by a 8 bits value)
(o)1 Present only if backcopy length is > 0 : backcopy offset size flag (if set : 2 bytes, if clear : 1 byte)
(o)7 MSB of backcopy offset
(o)8 LSB of backcopy offset
(o)8 If backcopy length == 31, this byte indicates backcopy length - 30
(o)8 If litteral length == 7, this optional byte indicates litteral length
   n litteral data

If both backcopy length and litteral length are 0 : end of stream. The first strip must start with a backcopy length of 0.
