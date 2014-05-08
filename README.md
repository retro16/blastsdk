*WARNING*

This project is currently evolving. Please do not base any new project on the current version of Blast SDK.

However, the monitor will stay stable, so you can use it without fear of bit rot.

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
asmx2 is copyright Bruce Tomlin (original source at http://xi6.com/projects/asmx/). Please see asmx2/README_BLAST.md for more information.


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

If you wish to build Sega CD ISOs, you must first extract the boot code from an original CD-ROM or from an ISO. Run ./extract_scdboot.sh to automate the process. You must then rebuild and reinstall blsbuild by running './setup.sh force'.


Parts
-----

The project is divided in many parts.

### asmx2 ###

This directory contains the patched asmx used by blast. Automatically installed by setup.sh.


### bdbridge ###

bdbridge is an Arduino project that allows the debugger to communicate with a Sega Genesis. See doc/debug_protocol.md for more information.


### bin2c ###

A small tool that converts a binary file to a C source. Used by extract_scdboot.sh.


### bls ###

The source for PC-side tools : bdb, blsbuild and d68k


### blsbuild ###

The builder. It works more or less loke make.

To use it, create a new directory with sources to be compiled, create a file named blsbuild.ini with correct content and run blsbuild from inside this directory. The tool will call the correct compilers and generate the final image, ready to use.


### bdb ###

The Blast Debugger. Allows to access the Genesis RAM with the help of the arduino board. The Blast Debugger Agent (BDA) must be included and properly initialized on the genesis side to enable debugging.

The tool is not finished and most features like breakpoints are still buggy, especially on the Sega CD.


### d68k ###

A 68k disassembler.


### doc ###

Documentation for the project


### inc ###

The assembler include directory. It is automatically installed by setup.sh. Provides assembler includes and macros.


### samples ###

Sample programs for the Blast SDK.

Currently only "monitor" and "blstest" work (mostly).


Status
------

The project is currently reworked. Especially blsbuild.

Things that work as expected (ready to use) :

 * asmx2 patches
 * d68k (may have still a few bugs)
 * bdbridge (tested only on arduino duemilanove)
 * bda (now works really well and is very compact)
 * bdb (still featureless, but the main idea is there)
 * bin2c

Things that will be removed

 * blsbuild (replaced by blsgen)

Things that will be available in the near future

 * blsgen
  * allows for more complex setups than blsbuild
  * smooth configuration format, based on markdown !
  * supports multi-section binaries
  * integration with a runtime loader to be able to reuse static RAM more efficiently (say goodbye to malloc)
  * will retain all the features of blsbuild, especially automatic memory mapping
  * will support SGDK officially (with restrictions)
  * will support Z80 (maybe SDCC for C support)
  * will contain hardcoded paths to ease integration in IDEs
  * will convert PNGs to genesis format, with the ability to extract image maps (using brute force) and palettes

 * blsloader
  * binary loader integrated in blsgen : blsgen generates loading macros for all binaries
  * each binary can be made of many sections from multiple sources
  * loading a binary is as simple as calling a macro
  * minimal runtime overhead
  * upcoming support for transparent CD-ROM loading
  * will be able to load code, GFX, sound, raw binaries in one bundle
  * each binary can target multiple memory chips
