The Blast Debugger Agent
========================

BDA is the piece of code that runs on the Genesis to implement the debugging
protocol. It is necessary to embed the BDA and setup level 2 exceptions
properly to make use of its features.

The BDA has only 2 roles : store registers and access memory. Please see
debugging_protocol for an in-depth explanation of how it works.


How to enable BDA in your program :
---------------------------------

1. Enable on cartridge or Sega CD programs

You will need to create 2 ASM files : bda.asm and bda_ram.asm.

bda.asm must contain these 2 lines :
    		include bls.inc
    		include bda.inc

bda_ram.asm must contain these 2 lines :
    		include	bls.inc
    		include	bda_ram.inc


Then, you must declare these files in the INI file with no special
requirement :

    [bda.asm]
    type=asm
    chip=cart
    
    [bda_ram.asm]
    type=asm
    chip=ram
    store=0


Then, ensure that level 2 interrupts are enabled in the VDP register #11
by setting the VDPEINT flag.

Finally, you must call bda_init at the beginning of your program before
enabling interrupts. This code sets up the gamepad port and, for SCD, copies
the agent in reserved RAM after the vector table.


2. Enable on RAM programs

Enabling BDA on RAM programs is easier since the agent is already resident on
the machine.

BDA resides in Sega-CD reserved RAM after $FFFD00, so just reserve this part
of RAM in your INI file :

    [scd_reserved]
    type=empty
    chip=ram
    addr=$FD00
    binsize=$300
    store=0

Then, ensure that level 2 interrupts are enabled in the VDP register #11
by setting the VDPEINT flag.

You must not call bda_init since initialization has already been done.

