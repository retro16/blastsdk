The Blast Exception Handler
===========================

BEH is a set of macros used to have clearer information in case of CPU
exceptions. When an exception occurs, the screen is cleared and displays
register values and a small stack trace.

When triggered, it displays 4 blocks : The upper-left block contains D0-D7
registers. The lower-left block contains A0-A7 registers. The beginning of the
right column contains the exception vector (2 bytes), followed by a dump of
the stack (beginning with SR and PC registers, as provided by the 68000).

To enable BEH, include beh.inc anywhere in your project. This code must be
present whenever an exception triggers, so you should place it in its own
file. The generated code is relocatable, but remember to update exception
vectors.
