Blast ! build system (blsbuild)
===============================

This tool allows to build a set of sources in various formats and generate a
binary suitable for the genesis or sega cd.

The tool reads a INI file (by default blsbuild.ini) and builds all the sources
defined in it.

Compilation phases
------------------

blsbuild operates in 5 phases :

 * A first pass determines the final size of each source

 * blsbuild computes the logical and physical layout of all sources. It means
   that the tool will automatically compute the optimal location of objects,
   whatever the type it is. You can layout ROM, RAM (68k and Z80), VRAM, PRAM
   and WRAM for logical locations and physical placement on cartridge /
   CD-ROM.
 
 * When all logical locations are known, a compilation pre-pass is done to
   extract all symbol values from sources.

 * Then, a final compilation + linking phase generates binaries.

 * Finally, the image file is built from all sources.

This approach helps the developper because the time-consuming operation of
ROM/RAM layout is totally automated (within some acceptable limits).

INI files
---------

blsbuild uses INI files to build a project.

### INI file format ###

The INI parser is somewhat strict : no extra spaces are allowed around the
equal sign, values are not space-trimmed and comments are not allowed after a
value. Maybe this will change in a future version.

Before data sections, there is a global (unnamed) section that holds global
values.


### Global options ###

cc=C compiler
cxx=C++ compiler
cflags=C and C++ compiler flags
ld=C linker
ldflags=linker flags
objcopy=path to objcopy
nm=path to nm
asmx=path to asmx

output=image file name
author=image author (keep short)
title=image title (keep short)
masterdate=MMDDYYYY mastering date
region=JUE (only one letter for SCD)
copyright=copyright / license information
serial=serial number
scd=0 for genesis, 1 for SCD mode 1, 2 for SCD mode 2


### Sections ###

Each section describes a source. If not overridden, the section name is the
source file name. For example, [main.asm] declares the "main.asm" file as a
source.


These are the available options for each source :

type=source type
chip=memory chip where this source will be located when in use
bus=bus used to access this resource (default : compute from chip+addr)
filename=source file name (default : section name)
align=memory alignment (default : depends on chip)
store=1 to store on physical medium (default), 0 to reserve space only
addr=target address (chip address) (default : computed automatically)
binsize=binary size once compiled (default : computed)
physaddr=address on the physical medium (default : computed)
physalign=alignment on physical medium (default : 2 for cart, complex on CD)
groups=dependency groups

Values for type :
 empty=empty memory
 bin=raw binary file
 asm=assembler file
 c_m68k=C source for m68k
 cxx_m68k=C++ source for m68k
 c_z80=C source for Z80 (SDCC)
 png=png file
 pngspr=png with sprite layout
 pngpal=16 color palette from PNG file

Values for chip :
 cart=cartridge ROM
 ram=main 68k RAM
 zram=Z80 RAM
 vram=Genesis video RAM
 pram=SCD program RAM
 wram=SCD word RAM

Values for bus :
 main=main 68k bus
 sub=sub 68k bus
 z80=Z80 bus


Chip and bus addresses
----------------------

blsbuild stores addresses internally as chip/address couples. Because the
genesis is a complex system with many CPUs (especially with the SCD attached),
one byte from one given memory chip may be accessed from many CPUs with
different addresses. The chip address describes the address inside a given
memory chip. For example, the address 0xFF0304 seen from the main CPU points
to the offset 0x0304 in the 64k RAM chip : 0xFF0304 is a bus address and
0x0304 is a chip address.

A more complex example would be the address 0x227 as seen from the Z80. It is
located at the offset 0x227 in the 8k RAM chip. If compiling for the Z80, the
chip address zram/0x227 would translate to 0x227. If compiling for the main
68k, the very same chip address would translate to 0xA00227.

An even more complex example occurs for banked memory. The chip address
cart/0x10840 translates to main/0x10840. But the Z80 only accesses the
cartridge in 32k banks : cart/0x10840 translates to the bus address
z80/0x8840/2 (Address 0x8840 using bank 2).


Automatic allocator
-------------------

blsbuild has an algorithm to automatically layout memory. If you omit the
"addr" property of sources, the allocator will compute them for you using a
first-fit algorithm. This means that it is able to put small elements in holes
between large ones in most cases.

Of course, you can force the address of some elements and leave the rest to
the automatic allocator : space between the fixed elements will be used.

The same thing applies for binsize (the final binary size). A crude algorithm
will be used to estimate the size of all binaries and used as base. Since the
size of a program is difficult to estimate, the algorithm may fail (with
warnings if binsize is too large, error if too small).


Groups
------

Group is a comma separated list of names. Two sources that share at least one
group cannot overlap. Use this to allow memory space reuse. A good example
would be different levels of a game : each level would define a group and some
resources would be useful only for one level while some other (like main
character sprite) should be present on all levels. 64 groups can be defined.

If groups is not defined, the resource is in all groups (no reuse)


Source symbols
--------------

A few symbols are generated from source sections
(examples for a source named [source.ext] )

SOURCE_EXT=logical address viewed from the current CPU
SOURCE_EXT_SIZE=size in bytes
SOURCE_EXT_BANK=bank if logical address is bank switched (for example ROM
reached from the Z80)
SOURCE_EXT_CHIP=target chip of the resource
SOURCE_EXT_ROM=physical address in ROM file (only for cartridge image)
SOURCE_EXT_SECT=CD-ROM sector
SOURCE_EXT_SECOFF=offset in CD-ROM sector
SOURCE_EXT_CONCAT=for concatenated sources, address relative to the first
element of the concatenation

All addresses are adjusted for the current CPU using chip-bus translations.


Concatenated resources
----------------------

This feature is somewhat hackish and has its rough edges, but you can
concatenate sources so they are seen as one big source by the allocator. In
the first source, specify "concat=next_source" to append nexy_source to the
first one. Sources must appear in the INI file in the order they are
concatenated.

You can concat sources targetting different chips : you can use the
bls_relocate macro to move the concatenated resource to its final location.

