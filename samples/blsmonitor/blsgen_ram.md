Displays a RAM dump on screen and allow to access RAM using the gamepad.
It also embeds BDA to enable debugger.

This is the RAM program version.

Instructions to build and run
=============================

Run from a cartridge/linker
---------------------------

Build the binary image

    blsgen blsgen_ram.md

Burn monitor_ram.bin on a cart or load it from your linker

See README.md for how to use


Run from blsmonitor
-------------------

Build the binary image

    blsgen blsgen_ram.md

Start blsmonitor on the genesis / sega cd

Run bdb (replace /dev/ttyUSB0 with your arduino serial port device / IP address)

    bdb /dev/ttyUSB0

Upload the binary image (this takes a long time)

    bdb > boot monitor.ram

Run the program

    bdb > go

See README.md for how to use


Section font.png.img
====================

 - chip ram

Binary main
===========

 - provides monitor.asm monitor_ram.asm font.png.img
 - bus main
 - include bls_init.md

Output monitor
==============

 - name **BLS MONITOR**
 - copyright **(C)2014 RETRO16**
 - file monitor.ram
 - region JUE
 - target ram

