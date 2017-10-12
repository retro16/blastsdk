The Blast ! Emulator debug protocol
===================================

This page describes the protocol used by the debugger to communicate with an
emulator.

This document describes differences / additions to the hardware protocol. Refer
to the hardware debugger documentation for the complete description.

Connection to the emulator
--------------------------

The emulator opens a network port. By default this is port 2613.

Monitor mode
------------

When entering monitor mode, the whole emulation is halted. You cannot halt only
the main or sub CPU like you would do on a real machine.

Comm words are not used for debug purposes.

Testing intermediate connectivity
---------------------------------

Version request will lead to different results to be able to tell the
difference between a real genesis and an emulator.

Send a version request :

    20 56 51 0A    " VQ\n"

The emulator will reply with a version and revision number

    20 65 31 30    " e10" (protocol for emulators, version 1, implementation 0)

Specific commands for emulator
------------------------------

