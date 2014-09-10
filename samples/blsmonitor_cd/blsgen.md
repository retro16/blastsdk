Displays a RAM dump on screen and allow to
read / write to RAM using the gamepad.

Source monitor_ram.asm
======================

 - chip ram

Section font.png.img
====================

 - chip ram

Source monitor.asm
==================

 - bus main

Binary main
===========

 - provides main.asm monitor.asm monitor_ram.asm font.png.img bda_ram.asm
 - bus main

Binary sub
==========

 - provides sub.asm

Output monitor
==============

 - name **Monitor**
 - file monitor.bin
 - region JUE
 - target scd

