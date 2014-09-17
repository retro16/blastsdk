Displays a RAM dump on screen and allow to access RAM using the gamepad.
It also embeds BDA to enable debugger.
This is the standalone genesis cart version.

Instructions to build and run
=============================

Build the binary image

    blsgen

Burn monitor.bin on a cart or load it from your linker

See README.md for usage

Section font.png.img
====================

 - chip cart

Binary main
===========

 - provides main.asm monitor.asm monitor_ram.asm font.png.img bda_ram.asm
 - bus main

Output monitor
==============

 - name **BLS MONITOR**
 - copyright **(C)2014 RETRO16**
 - file monitor.bin
 - region JUE
 - target gen

