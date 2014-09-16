*WARNING*

This project is in pre-alpha state. Some parts work, but it is far from being complete.


The Blast ! Software Development Kit
====================================


What is Blast ! SDK
-------------------

This is a collection of tools that will allow you to build and debug programs for the Sega Genesis / Mega Drive.
Currently the tools allow to build ASM (thanks to asmx) and C programs, statically link executables with full dependencies resolution and automatically layout RAM, ROM and physical medium. A debugger is also provided, allowing to communicate with real hardware using an arduino board plugged in the genesis pad or serial socket.


Why Blast ! SDK
---------------

The name is an obvious reference to blast processing, a marketing term associated to the Genesis (whether it is real or not is beyond the scope of this document).
The primary aim is to develop for the Sega CD and try to reach its maximum potential. I do believe that realtime textured 3D rendering is possible, and I need a tool to test it.
Then, I found that the homebrew scene of the 16 bits Sega machines is still alive and has some tools and knowledge, but I did not find tools needed for larger scale projects and (relatively) comfortable development.


License
-------

Licensed under MIT license. Please see LICENSE file.
asmx2 is copyright Bruce Tomlin (original source at http://xi6.com/projects/asmx/). The provided asmx2 is not the original version and has substantial modifications. Please see asmx2/README_BLAST.md for more information.


Supported systems
-----------------

The latest Ubuntu, Debian, Gentoo and Cygwin have been tested. Other platforms may or may not work.

Arduino compatible systems should work. Only Arduino Duemilanove has been extensively tested.


Dependencies
------------

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
 
Quick start
-----------

Run ./setup.sh to build and install all tools automatically. If run as root, it will install to /usr/local, which is usually in your PATH, so it will be installed globally for all users. If not installed in /usr/local, remember to add the bin folder in your PATH.

When tools are built, use a project in the samples directory as a starting point.

If you wish to build Sega CD ISOs, you must first extract the boot code from an original CD-ROM or from an ISO. Run tools/extract_scdboot.sh to automate the process. You must then rebuild and reinstall blsbuild by running './setup.sh force'.


Parts
-----

The project is divided in many parts.

### tools/asmx2 ###

This directory contains the patched asmx used by blast. Automatically installed by setup.sh.


### tools/bdbridge ###

bdbridge is an Arduino project that allows the debugger to communicate with a Sega Genesis. See doc/debug_protocol.md for more information.


### tools/bin2c ###

A small tool that converts a binary file to a C source. Used by extract_scdboot.sh.


### tools/bls ###

The source for PC-side tools : bdb, blsgen and d68k


### tools/bls/blsgen ###

The builder. It works more or less like make.

 * smooth configuration format, based on markdown !
 * supports multi-section binaries
 * integration with a runtime loader to be able to reuse static RAM and globals more efficiently (say goodbye to malloc)
 * automatic memory mapping, even between different CPUs (remaps shared globals)
 * hardcoded paths to ease integration in IDEs
 * converts PNGs to genesis format, with the ability to extract palettes
 * Generates fully static and non-relocatable binaries using a very slow (but very efficient) multi-pass compilation approach
 * will support SGDK in the future (with restrictions)
 * will support Z80 (maybe SDCC for C support)

### tools/bls/bdb ###

The Blast Debugger. Allows to access the Genesis RAM with the help of the arduino board. The Blast Debugger Agent (BDA) must be included and properly initialized on the genesis side to enable debugging.

Some features are still missing (like sub CPU breakpoints or print() support) but it is enough for day to day work if you know 68000 assembly.


### tools/bls/d68k ###

A 68k disassembler.


### doc ###

Documentation for the project


### inc ###

The assembler include directory. It is automatically installed by setup.sh. Provides assembler includes and macros.


### samples ###

Sample programs for the Blast SDK.

Currently only "blsmonitor" and "blsgentest" work.


Status
------

Things that work as expected (ready to use) :

 * asmx2 patches
 * d68k (cycle count is still wrong in some cases)
 * bdbridge (tested only on arduino duemilanove)
 * bda (now works really well and is very compact)
 * bdb (needs improvements on)
 * bin2c
 * blsgen
