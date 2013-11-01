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


Quick start
-----------

Run ./setup.sh to build and install all tools automatically. If run as root, it will install to /usr/local, which is usually in your PATH, so it will be installed globally for all users. If not installed in /usr/local, remember to add the bin folder in your PATH.

When tools are built, use a project in the samples directory as a starting point.

If you wish to build Sega CD ISOs, you must first extract the boot code from an original CD-ROM or from an ISO. Run ./extract_scdboot.sh to automate the process. You must then rebuild and reinstall blsbuild by running './setup.sh force'.


Status
------

The project is still in early development stages.

* blsbuild code is ugly
* documentation is sparse
* most features don't work
* the SDK provides virtually no functions
* C support is totally untested
* Z80 is totally untested
* exception handling is problematic
* Debug agent randomly crashes on Sega CD

