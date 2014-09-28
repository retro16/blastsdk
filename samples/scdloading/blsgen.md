Stress test for binary loading on the Sega CD

This is the european Sega Mega-CD version.


--------------------------------------

Main CPU binaries
=================


Binary main
-----------

 - provides main.asm bls_init.asm bda_ram.asm
 - bus main

Binary main_common
------------------

Press A to start main_bin1, B to start main_bin2 and C to start main_bin3

 - provides main_common.asm console.asm console_ram.asm font.png.img font.png.pal
 - bus main

Section font.png.img
--------------------

 - chip vram
 - addr $400

Binary main_bin1
----------------

 - provides main_bin1.c
 - bus main

Binary main_bin2
----------------

 - provides main_bin2.c
 - bus main

Binary main_bin3
----------------

 - provides main_bin3.asm
 - bus main


--------------------------------------

Sub CPU binaries
================


Binary sub
----------

 - provides sub.asm blsload.asm
 - bus sub

Binary sub1
-----------

 - provides sub1.asm
 - bus sub

Binary sub2
-----------

 - provides sub2.c
 - bus sub

Binary sub3
-----------

 - provides sub3.c
 - bus sub


--------------------------------------

Output monitor
==============

 - name **Monitor**
 - file monitor_scd_eu.iso
 - region E
 - target scd1

