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

lz68k is a compression format inspired by LZ4, but modified for easier decompression on a m68k CPU and smaller offsets. Input and output are byte streams.

bits field
                           4 backcopy offset (0 == extension bytes)
   6 litteral length       2 litteral length (3 == extension byte)
   2 backcopy length = 0   2 backcopy length (0 == no backcopy, 3 == extension bytes)
   8 Backcopy offset - 16 (if MSB set, split over 2 bytes)
   8 Backcopy offset bits 7-14
   8 Backcopy length if length == 3 in header
   8 litteral length
   n litteral data

If both backcopy length and litteral length are 0 : end of stream. The first strip must start with a backcopy length of 0.
