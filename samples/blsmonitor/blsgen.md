Displays a RAM dump on screen and allow to
read / write to RAM using the gamepad.

Source monitor_ram.asm
======================

 - chip ram

Section font.png.img
====================

 - chip cart

Section mainstack
=================

 - chip ram
 - format empty
 - size $100

Binary main
===========

 - provides main.asm monitor.asm monitor_ram.asm font.png.img mainstack bda_ram.asm

Output monitor
==============

 - name **Monitor**
 - file monitor.bin
 - region JUE
 - target ram

