This is a test program for bdp written in C.
It prints "Hello C world\n" in bdb and returns to monitor mode.

Instructions to build and run
=============================

Run from a cartridge/linker
---------------------------

Build the binary image

    blsgen

Burn bdphello_c.bin on a cart or load it from your linker

Run bdb (replace /dev/ttyUSB0 with your arduino serial port device / IP address)

    bdb /dev/ttyUSB0

Run the program

    bdb > go
    Hello C world
    TRAP #07

TRAP #07 indicates that the program switched to monitor mode on its own.
This is caused by the ENTER_MONITOR macro call.

Run from blsmonitor
-------------------

Build the binary image

    blsgen

Start blsmonitor on the genesis / sega cd

Run bdb (replace /dev/ttyUSB0 with your arduino serial port device / IP address)

    bdb /dev/ttyUSB0

Upload the binary image (this takes a long time)

    bdb > boot bdphello_c.bin
    Ready to boot
    d0:00722700 d1:0000FFFF d2:00009300 d3:00000000 d4:00000164 d5:00004537 d6:00000045 d7:0000FFFF
    a0:00FF002B a1:00FF0896 a2:00FF002A a3:00000000 a4:00C00000 a5:00C00004 a6:FFFFD400 a7:00FFFD00
    pc:00FF0000 sr:2700

Run the program

    bdb > go
    Hello C world
    TRAP #07

TRAP #07 indicates that the program switched to monitor mode on its own. This
is caused by the ENTER_MONITOR macro call.


Binary **main**
===============

 - provides main.c
 - include bdp.md


Output **bdphello_c**
=====================

Compiled binaries:

 - binaries main

Output properties:

 - name **BDP hello**
 - copyright **(C)2014 RETRO16**
 - target ram
 - region JUE
 - file bdphello_c.bin


