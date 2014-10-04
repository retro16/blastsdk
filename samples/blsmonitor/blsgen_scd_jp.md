Displays a RAM dump on screen and allow to access RAM using the gamepad.
It also embeds BDA to enable debugger.

This is the japanese Sega Mega-CD version.

Instructions to build and run
=============================

Run from a CD-ROM
-----------------

Build the binary image

    blsgen blsgen_scd_eu.md

Burn monitor_scd_jp.iso on a non-rewritable CD-ROM

Boot the CD on your Sega CD

See README.md for how to use


Run from blsmonitor
-------------------

Since this program does not access the CD drive, it can be run from
the debugger without burning it to a CD.

Build the binary image

    blsgen monitor_scd_jp.md

Start blsmonitor on the genesis / sega cd

Run bdb (replace /dev/ttyUSB0 with your arduino serial port device / IP address)

    bdb /dev/ttyUSB0

Upload the binary image (this takes a long time)

    bdb > boot monitor_scd_jp.iso

Run the program

    bdb > go

See README.md for how to use

Section font.png.img
====================

 - chip ram

Binary main
===========

 - provides main.asm monitor.asm monitor_ram.asm font.png.img
 - bus main
 - include bda.md beh.md

Binary sub
==========

 - provides sub.asm
 - bus sub

Output monitor
==============

 - name **Monitor**
 - file monitor_scd_jp.iso
 - region J
 - target scd1

