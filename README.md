*News* The project has reached beta stage and is now usable.

The Blast ! Software Development Kit
====================================


What is Blast ! SDK
-------------------

This is a collection of tools that will allow you to build and debug programs
for the Sega Genesis / Mega Drive.  Currently the tools allow to build ASM
(thanks to asmx) and C programs, statically link executables with full
dependencies resolution and automatically layout RAM, ROM and physical medium.
A debugger is also provided, allowing to communicate with real hardware using
an arduino board plugged in the genesis pad or serial socket.


Why Blast ! SDK
---------------

The name is an obvious reference to blast processing, a marketing term
associated to the Genesis (whether it is real or not is beyond the scope of
this document).  The primary aim is to develop for the Sega CD and try to reach
its maximum potential. I do believe that realtime textured 3D rendering is
possible, and I need a tool to test it.  Then, I found that the homebrew scene
of the 16 bits Sega machines is still alive and has some tools and knowledge,
but I did not find tools needed for larger scale projects and (relatively)
comfortable development.


License
-------

Licensed under MIT license. Please see LICENSE file.  asmx2 is copyright Bruce
Tomlin (original source at http://xi6.com/projects/asmx/). The provided asmx2
is not the original version and has substantial modifications. Please see
asmx2/README_BLAST.md for more information.


Supported systems
-----------------

The latest Ubuntu, Debian, Gentoo and Cygwin have been tested. Other platforms
may or may not work.

Arduino compatible systems should work. Only Arduino Duemilanove has been
extensively tested.


Quick how-to
============

If you want to be ready as quickly as possible, here are the steps :


Building Blast
--------------

To build blast, the following packages are needed :

* General scripting
 * bash
 * GNU make
 
* Building GCC
 * bison
 * yacc
 * gmp
 * mpc
 * mpls
 
* Building blsbuild
 * libpng
 
* Building bdb
 * readline

* Building bdbridge
 * Arduino 1.x

### Cygwin dependencies ###

To build under Cygwin, you will need to install the following packages :

 - Devel/bison
 - Devel/byacc
 - Devel/gcc-core
 - Devel/gcc-g++
 - Devel/git
 - Devel/make
 - Devel/pkg-config
 - Devel/subversion
 - Graphics/libpng-devel
 - Libs/libgmp-devel
 - Libs/libmpc-devel
 - Libs/libmpfr-devel
 - Libs/libreadline-devel
 - Web/wget


Building the bridge
-------------------

You will need an arduino board and a female 9 pin plug that fits the gamepad
port. You may reuse an old serial cable extension cord for PC.

Make a cable with the following pinout :

    Arduino pin     Genesis pin
    
        3 <------------> 7     Looking at the
        4 <------------> 1     console socket :
        5 <------------> 2     -------------
        6 <------------> 3     \ 1 2 3 4 5 /
        7 <------------> 4      \ 6 7 8 9 /
        8 <------------> 6        -------
        9 <------------> 9
      GND <------------> 8

Genesis pad pin 5 (+5V) must *not* be connected.

Then, download the Arduino IDE and open the file tools/bdbridge/bdbridge.ino.
Compile it and upload it to the arduino : you should now have a functional
bridge. You can test if the bridge works by typing the 'bridge' command in bdb.

By default, you must plug the cable in the second player gamepad port.

It is possible to use the EXT port of older models, but it requires small
changes in the source.

 
Installing the software toolchain
---------------------------------

Run ./setup.sh to build and install all tools automatically. If run as root, it
will install to /usr/local, which is usually in your PATH, so it will be
installed globally for all users. If not installed in /usr/local, remember to
add the bin folder in your PATH.

When tools are built, use a project in the samples directory as a starting
point.

If you wish to build Sega CD ISOs, you must first extract the boot code from an
original CD-ROM or from an ISO. Run tools/extract_scdboot.sh to automate the
process. You must then rebuild and reinstall blsbuild by running './setup.sh
force'.


Beginning with the toolchain : samples
--------------------------------------

The kit provides a few samples in the "sample" directory. You should begin with
"blsmonitor" as it is the most stable sample and it can be a quite useful tool
for more advanced hacking.

This tool does 2 things : it will display a memory dump on screen (with a few
controls to hack around) and it will install the Blast Debugger Agent.

The Blast Debugger Agent (BDA for short) is a small resident program that talks
with the debugger on the PC via the arduino board. See doc/debug_protocol.md
for more details on how it works.

The way you will compile and run programs depend on which hardware you target:

### Running blsmonitor on the Genesis with a linker / rom flash

    cd samples/blsmonitor
    blsgen

If everything went well (it should !) you will have a file named monitor.gen.
This file can be directly run on an emulator or burnt on a ROM or run by any
linker like the EverDrive.

### Running blsmonitor on the Sega CD / Mega-CD

    cd samples/blsmonitor
    blsgen blsgen_scd_us.md

Of course, you must choose the correct region for your particular region:
blsgen_scd_us.md, blsgen_scd_jp.md or blsgen_scd_eu.md.

You can then burn the generated blsgen_scd_us.iso or use an emulator to run the
file.


Debugging
---------

Once the monitor runs on your genesis, you can connect to it using the
debugger. First you must find on which device your arduino is connected.
Usually this is /dev/ttyUSB0 under Linux or /dev/ttyS3 under Windows. The
bridge uses serial communication at 115200 bps. Alternatively, you can use
the arduino ethernet shield (see bdbridge.ino source for more detail).

If your arduino has a LED on pin 13, you can see the status of the bridge:
at first boot, the LED is lit, then it enters half-bright mode when it
detects that BDA is running on the genesis. Half-bright means that the
bridge is running its main loop and that it is fully functional.

Start the debugger :

    $ bdb /dev/ttyUSB0 
    Blast ! debugger. Connected to /dev/ttyUSB0

The prompt already shows some info : the 'M' indicates that you control the
main CPU (the one in the genesis, not the one in the sega CD). When in caps,
the CPU is running the program and when displaying a small 'm', it is in
monitor mode (execution is paused).

Let's stop the CPU and have a look at the inside of the genny !

    bdb M> stop
    bdb m>

Dump the beginning of the main RAM:

    bdb m> dump $FF0000 $20
    FF0000: 00 FF 00 00 00 FF 00 00 00 00 00 00 00 00 33 7F   ..............3.
    FF0010: 01 01 0C 22 0E 44 0E 66 0E 88 0E EE 0A AA 08 88   ...".D.f........

Since blsmonitor shows RAM on screen by default, you should see the same data
on screen and in the dump (or at least something very similar).

Now let's have a look at the registers of the CPU:

    bdb m> reg
    main cpu registers :
    d0:0000FFFF d1:00000100 d2:00009300 d3:00000000 d4:00000164 d5:00004537 d6:00000045 d7:0000FFFF 
    a0:FFFFFD08 a1:00FF0896 a2:FFFFFB20 a3:00000000 a4:00C00000 a5:00C00004 a6:FFFFD400 a7:FFFFFCFC 
    pc:00FF0B28 sr:2000 [   S 0           ]

The elements between brackets after the sr register are the flags in readable
form (Z for zero, X for X flag, C for carry, interrupt level, ...).

You can also disassemble any part of the memory. We know that PC is at $FF0B28,
so let's disassemble starting at this address

    bdb m> d68k $FF0B28 $16
    aFF0B28 BRA.B   aFF0B28                 ; FF0B28 10
            LEA     $C00000.L, A4           ; FF0B2A 12
            LEA     $C00004.L, A5           ; FF0B30 12
            MOVE.W  #$8104, (A5)            ; FF0B36 12
            MOVE.W  #$8F02, (A5)            ; FF0B3A 12

By default, with no address, d68k only disassembles the instruction pointed by
the PC register. Handy to quickly check where you are:

    bdb m> d68k
    Starting at PC (00FF0B28)
    aFF0B28 BRA.B   aFF0B28                 ; FF0B28 10

The last column is the cycle count for each instruction (it is still buggy,
please report any problems about this feature).

If you started bdb in the directory where you built the source with blsgen, it
can read symbols from the build_blsgen/blsgen.md and display them in d68k.

You can run the code step by step ("step" or "s" command), run to the next
instruction with "next" or "n" command (useful to skip loops or subroutine
calls) and you can skip totally the current instruction without executing it
with the "skip" instruction.

    bdb m> step
    main cpu registers :
    d0:0000FFFF d1:00000100 d2:00009300 d3:00000000 d4:00000164 d5:00004537 d6:00000045 d7:0000FFFF 
    a0:FFFFFD08 a1:00FF0896 a2:FFFFFB20 a3:00000000 a4:00C00000 a5:00C00004 a6:FFFFD400 a7:FFFFFCFC 
    pc:00FF0B28 sr:2000 [   S 0           ]
    (next) aFF0B28 BRA.B   aFF0B28                 ; FF0B28 10
    bdb m> skip
    (skip)         BRA.B   $FF0B28
    (next)         LEA     $C00000.L, A4           ; FF0B2A 12
    bdb m> step
    main cpu registers :
    d0:0000FFFF d1:00000100 d2:00009300 d3:00000000 d4:00000164 d5:00004537 d6:00000045 d7:0000FFFF 
    a0:FFFFFD08 a1:00FF0896 a2:FFFFFB20 a3:00000000 a4:00C00000 a5:00C00004 a6:FFFFD400 a7:FFFFFCFC 
    pc:00FF0B30 sr:2000 [   S 0           ]
    (next)         LEA     $C00004.L, A5           ; FF0B30 12


You can set any register, for example, setting the PC at $FF0000:

    bdb m> set pc $FF0000

You can also make byte, word or long read/writes on the bus, using r and w
commands.

To switch between main and sub CPU, use "main" and "sub" commands. These can
be prefixed to any other command. For example, to switch on sub cpu AND stop it
at the same time, you can type this:

    bdb m> substop
    Switch to sub cpu
    bdb s>

Finally, you can repeat the last command you typed by simply entering an empty
command (pressing return). Useful for fast step by step.


Booting a program in bdb
------------------------

bdb can boot RAM programs and CD-ROM images built with blsgen (no, it cannot
run commercial games this way !)

Use the boot command, then start the CPU. Remember to also start the sub CPU
for Sega CD programs.

    bdb M> boot monitor_scd_eu.iso
    CD-ROM image. IP=076E-7FFF (30866 bytes). SP=1800-1FFF (2048 bytes).
    Entering monitor mode on main CPU
    Freezing sub CPU
    Clearing communication flags ...
    Clearing BDP buffer ...
    Uploading IP (30866 bytes) ...
    Setting main CPU registers.
    Uploading simulated BIOS ...
    BDA sub code found (008E bytes), upload it at 000106 and reset CPU
    TRACE vector at 0000011A
    LEVEL2 vector at 0000016C
    TRAP #7 vector at 0000017A
    Reset entry at 00000106
    Uploading SP (2048 bytes) ...
    Setting sub CPU registers.
    Ready to boot.
    Imported 564 symbols.
    main cpu registers :
    d0:0000FFFF d1:00000100 d2:00009300 d3:00000000 d4:00000164 d5:00004537 d6:00000045 d7:0000FFFF 
    a0:FFFFFD08 a1:00FF0896 a2:FFFFFB20 a3:00000000 a4:00C00000 a5:00C00004 a6:FFFFD400 a7:FFFFD000 
    pc:FFFF056E sr:2700 [   S 7           ]
    bdb m> go

(debug output may vary).

You can also directly upload a program on bdb command-line:

    $ bdb /dev/ttyUSB0 monitor_scd_eu.iso 
    Blast ! debugger. Connected to /dev/ttyUSB0

    CD-ROM image. IP=076E-7FFF (30866 bytes). SP=1800-1FFF (2048 bytes).
    Entering monitor mode on main CPU
    Freezing sub CPU
    Clearing communication flags ...
    Clearing BDP buffer ...
    Uploading IP (30866 bytes) ...
    Setting main CPU registers.
    Uploading simulated BIOS ...
    BDA sub code found (008E bytes), upload it at 000106 and reset CPU
    TRACE vector at 0000011A
    LEVEL2 vector at 0000016C
    TRAP #7 vector at 0000017A
    Reset entry at 00000106
    Uploading SP (2048 bytes) ...
    Setting sub CPU registers.
    Ready to boot.
    Imported 564 symbols.
    main cpu registers :
    d0:0000FFFF d1:00000100 d2:00009300 d3:00000000 d4:00000164 d5:00004537 d6:00000045 d7:0000FFFF 
    a0:FFFFFD08 a1:00FF0896 a2:FFFFFB20 a3:00FFFF56 a4:00C00000 a5:00C00004 a6:FFFFD400 a7:FFFFD000 
    pc:FFFF056E sr:2700 [   S 7           ]
    bdb m>

When booting CD-ROM images through BDB, it automatically replaces the BIOS with
a version that uses the bridge to simulate the CD-ROM. This way, you can
basically burn the blsmonitor once, boot it and develop your program on real
hardware using the drive simulator of BDB.

This feature is still new, and often interferes with other debugging commands,
so do not use any command while your program accesses the CD-ROM drive.


Directory structure
===================

The project is divided in many subdirectories.

### tools/asmx2 ###

This directory contains the patched asmx used by blast. Automatically installed
by setup.sh.


### tools/bdbridge ###

bdbridge is an Arduino project that allows the debugger to communicate with a
Sega Genesis. See doc/debug_protocol.md for more information.


### tools/bin2c ###

A small tool that converts a binary file to a C source. Used by
extract_scdboot.sh.


### tools/bls ###

The source for PC-side tools : bdb, blsgen and d68k


### tools/bls/blsgen ###

The builder. It works more or less like make.

 * smooth configuration format, based on markdown !
 * supports multi-section binaries
 * integration with a runtime loader to be able to reuse static RAM and globals
   more efficiently (say goodbye to malloc)
 * automatic memory mapping, even between different CPUs (remaps shared
   globals)
 * hardcoded paths to ease integration in IDEs
 * converts PNGs to genesis format, with the ability to extract palettes
 * Generates fully static and non-relocatable binaries using a very slow (but
   very efficient) multi-pass compilation approach
 * will support SGDK in the future (with restrictions)
 * will support Z80 (maybe SDCC for C support)

### tools/bls/bdb ###

The Blast Debugger. Allows to access the Genesis RAM with the help of the
arduino board. The Blast Debugger Agent (BDA) must be included and properly
initialized on the genesis side to enable debugging.

Some features are still missing (like sub CPU breakpoints or print() support)
but it is enough for day to day work if you know 68000 assembly.


### tools/bls/d68k ###

A 68k disassembler.


### doc ###

Documentation for the project


### include ###

The assembler, C and blsgen include directory. It is automatically installed by
setup.sh. Provides all the features you will access as a SDK user.

See the `*.md` files inside for more info on how to use each part of the library.


### src ###

Sources of the SDK running on the console. Automatically handled by setup and
`*.md` files in include directory.


### samples ###

Sample programs for the Blast SDK.

Currently only "blsmonitor" and "blsgentest" work as intented.


Status
------

Things that work as expected (ready to use) :

 * asmx2 patches
 * d68k (cycle count is still wrong in some cases, some instructions may be
   missing)
 * bdbridge (tested only on arduino duemilanove)
 * bda
 * bdb
 * bin2c
 * blsgen
 * libraries in include directory

Of course, there are bugs, please report them. Remember that this project is
less serious than big compilers or tools (like GCC or bash), so expect some
rough edges !

And any contributions will be greatly appreciated.

