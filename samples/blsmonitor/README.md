BLS Monitor
===========

Displays a RAM dump on screen and allow to access RAM using the gamepad.
It also embeds BDA to enable debugger.

The screen is made of 2 parts : the address line and the memory dump.

Memory dump
-----------

This memory dump displays the content of memory in real time. To scroll the
view, hold Start and press up/down to scroll line per line or left/right to
scroll one screen at a time.

Address line
------------

This line looks like this :

    FF0000 00000000 2

The first element is the current address. The second element is the data field.
The third element is the word size.

To read a value from memory, first specify an address using the D-pad, then
specify a word size (1 for byte, 2 for word, 4 for long word). When ready,
press C on the gamepad (the background should flash green briefly) and the
value will be shown in the data field. The value will be sign-extended to fill
the display.

To write a value in memory, set the address in the first field, then the value
in the second field (words and bytes are right-aligned), set the word size
(1 for byte, 2 for word, 4 for long) and press B (the background will flash red
briefly).

To execute code at a target address, set the address in the address field, set
word size to B and press B on the gamepad. The CPU will execute a JSR to the
address.

Targets
=======

5 versions for different targets are provided :

 - blsgen.md : Genesis cartridge
 - blsgen_ram.md : Genesis RAM program (runs on genesis and CD)
 - blsgen_scd_eu.md : European Mega CD
 - blsgen_scd_jp.md : Japanese Mega CD
 - blsgen_scd_us.md : US Sega CD

See the respective blsgen_*.md files for build instructions.
