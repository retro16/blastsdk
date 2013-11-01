Blast monitor
=============

This program displays memory on screen and allows to read and write to the bus.

Since it embeds BDA, BEH and an exception jump table in RAM, it is the perfect boot program to debug other programs.


Display
-------

The first line displays an address, a value and a word size.
The other lines display a dump (at startup, it points at the beginning of RAM).


Usage
-----

To scroll the dump, hold start and press the d-pad : up and down to scroll one line, left and right to scroll one page.

To set the dump address, enter an address in the address field and press button A.

To read values, enter the address in the address field and press button C. Press button B to write.


To execute a subprogram at an address, set the address in the VALUE field (second field), set word size to 'B' and press C. The CPU will do a JSR to the specified address.
With a lot of patience, it is a good way to upload a small routine and test it.

Word sizes
----------

 * 1 = byte access. Only the first byte of value is used.
 * 2 = word access. Only the first word of value is used.
 * 4 = long access.
 * B = branch mode.
