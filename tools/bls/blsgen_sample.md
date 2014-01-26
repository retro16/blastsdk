Sources
=======

A source references one file or one element. One source may generate one or more sections.

Source src1
-----------

 - name src1.asm
 - format asmx


Source src2.c
-------------

 - format gcc
 - optimize 2 (Pass **-O2** to gcc)


Section mybuffer
----------------

Since no source exists, create an implied source.
For implied sources, the last dot separates source name and section name.

Since there is no last dot here, the section will have an empty name.
By default, sources with no extension have type "empty".

 - addr $0
 - chip ram
 - size $300


Sections
========

A section is a piece of binary data that targets a specific address in a specific chip.

Before inclusion in a binary, a section may be transformed (compressed, encrypted or whatever transformation). This transformation is reverted by the runtime loader (uncompressed, decrypted, ...) while being placed into the section's final place.


Section src1
------------

Force src1 position. When loading function will be called, executable code will be copied into RAM.

 - addr $800
 - chip ram
 
 
Section src2.c.text
-------------------

Put this binary at a special address in ROM :

 - addr $300
 - chip cart


Section src2.c.data
-------------------

Put globals at a special address in main RAM :

 - addr $2000
 - chip ram

Override size :

 - size $300


Section src2.c.bss
------------------

Force BSS address in a fixed place in RAM :

 - addr $2300
 - chip ram


Section myfont.png.img
----------------------

 - addr $0
 - pfmt lzo (compress to lzo. BLS loader will decompress at runtime)
 
(png implies chip vram by default)


Binaries
========

Binaries are units loaded at once. One binary can reference many sections and one section can be referenced by many binaries.

In case of multiple references, sections will be kept at one place for carts and duplicated for CD-ROMs.


Binary mainbinary
-----------------

Providing a section makes the loader load it
 - provides myfont.png.img
 
Providing whole sources
 - provides src1, src2.c

A binary may provide or use sources, sections or other binaries. Sepcifying a source or a binary eventually adds all its sections.

Using a section, a source or a binary means that it is already present and must not be touched.
This example is probably unuseful since using any symbol from a section will imply "uses".
 - uses blsload.asm


Outputs
=======

An output defines an image of a physical medium. Many outputs can be generated from one project with different binaries and entry points, for example to generate a Genesis and a Sega-CD version of the same game.


Output mygame.img
-----------------

 - target gen (or scd or vcart, deduced from output name if ending in img, gen, iso or vcart)
 - region JUE
 - copyright 2014 GPL
 - author Retro 16
 - binaries mainbinary blsload.asm splash.c copyright.png
 
 - entrypoint splash
