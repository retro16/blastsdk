q                        quit
flush                    flush genesis input
r <b|w|l>[len] <addr>    read len bytes/words/longs at addr. N is in bytes. If
                         ommited, reads one.
w <b|w|l> <addr> <val>   writes val at specified address.

sendfile <addr> <file>   writes a file in RAM
go                       resumes execution
ping                     handshake with the monitor
regaddr                  sets the address of the CPU register table
reg                      display value of all registers


Naming conventions :

BDB (debugger) : the debugger (the interactive program running on the PC)
BDP (protocol) : the protocol and its API (sendmem, readword, ...)
BDA (agent) : protocol implementation on the main CPU - triggered by exceptions and sub CPU comm bit
BDS (sub agent) : protocol implementation on the sub CPU - triggered by exceptions, level2 + main CPU comm bit
BDE (bios emulator) : program loaded at $200-$5E00 simulating BIOS calls
bridge : the arduino program that translates USB/serial/ethernet streams from/to genesis gamepad signals.

How it works :

Compilation can generate images with or without BDA. BDA must be inserted in IP for SCD and in ROM for cart images.

On boot, bda_init will copy all required parts to correct locations, then its own RAM can be freed. It means that IP can be overwritten without problems once bda_init has returned.

There are many kinds of images that can be built using Blast SDK :

 1. Raw cart images
These images have no debugging facilities. Use this to generate "masters" for the genesis. To generate them, just generate a cart with blsbuild and never include "bda.inc".

 2. Raw CD images
These images are ISO ready to be burnt for CD. They have no debugging facilities. To generate them, just generate a CD image with blsbuild and never include "bda.inc".

 3. Debugging cart images
These images can be debugged using BDB. To generate them, include bda.inc in any resource and call bda_init at boot. RAM after $FFFFA0 cannot be used but blsbuild should automatically take that into account while building.

 4. Debugging CD images
These images can be debugged using BDB. To generate them, include bda.inc at the end of your IP and call bda_init at boot.

 5. Virtual cart images
This is a special kind of cartridge image : the aim of this kind of images is to run genesis programs on Sega CD RAM.

There are limitations :

 * upper genesis RAM (>$FFFD00) is reserved
 * the image cannot be larger than 256KB
 * addresses are translated to Sega CD WRAM ($200000)
 * Interrupts are slower due to an extra level of indirection
Since these images are not in the cartridge address space, they cannot be flashed on real cartridges.

