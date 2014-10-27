Stress test for binary loading on the Sega CD

This is the european Sega Mega-CD version.


--------------------------------------

Main CPU binaries
=================


Binary main
-----------

 - provides main.asm
 - bus main

 - include bls_init.md
 - include bdp.md

Section plane_a
---------------

 - chip vram
 - format empty
 - addr $E000
 - size $800

Section plane_b
---------------

 - chip vram
 - format empty
 - size $800
 - align $2000

Section hscroll_table
---------------------

 - chip vram
 - format empty
 - size $800
 - align $400

Section sprat
-------------

 - chip vram
 - format empty
 - size $800
 - align $400



Binary blast_splash
-------------------

 - provides blast_splash.png plane_b hscroll_table sprat
 - bus main

Section blast_splash.png.map
----------------------------

 - chip vram
 - addr $E000 (put on plane_a)
 - size $800



Binary text
-----------

 - provides font.png.img text.c bls_vdp.asm text.txt
 - bus main


Section font.png.img
--------------------

 - chip vram
 - addr $400 (align to ascii characters)

Source text.txt
---------------

 - chip ram
 - format raw



--------------------------------------

Sub CPU binaries
================


Binary sub
----------

 - provides sub.asm
 - bus sub
 - include blsload_sub.md bdp_sub.md



--------------------------------------

Output monitor
==============

 - name **Monitor**
 - file scdloading_eu.iso
 - region E
 - target scd2
 - binaries main sub

