The Blast ! Debugger protocol
=============================

Electrical specification.
-------------------------

Connected receiver must be open collector, with no pull-up. For output, pull
the pin to low or release it for high. For input, just read logic value.  This
will avoid problems when switching direction.

Port mapping / pinout

Looking at the console socket :

     -----------
    \ 1 2 3 4 5 /
     \ 6 7 8 9 /
       -------
    
    Bit Pin Ard Name Pad0  Pad1  Serial Monitor
     0  1   4   D0   Up    Up    -      D0
     1  2   5   D1   Down  Down  -      D1
     2  3   6   D2   0     Left  -      D2
     3  4   7   D3   0     Right -      D3
     4  6   8   TL   A     B     Tx     Ack
     5  9   9   TR   Start C     Rx     Busreq
     6  7   3   TH   Sel   Sel   INT    Clock
     7  NC  -   -    ?     ?     -      -
     -  8   GND GND  -     -     -      -
     -  5   NC  Vcc  -     -     -      -


Byte transmission format
------------------------

The debug mode communicates via D-Pad data lines, passing 4 bits per clock.  TH
is the clock, read on edge. Format is big nybble first. For example, to pass
0xF3, digit F would be passed first, then 3. Each nybble is acknowledged by a
TL transition. 5V is logic high and 0V is logic low.

Example transmitting 0xA3 to the genesis :

                           ___________________
    D0 ---________________/                   ------ output
          ____________________________________
    D1 ---                                    ------ output
                         
    D2 ---____________________________________------ output
          ________________     
    D3 ---                \___________________------ output
          ____                      __________
    TH ---    \____________________/          ------ output
          _______                     ________
    TL ---       \___________________/        ------ input
          _                               ____
    TR --- \_____________________________/    ------ output
    

Example receiving 0xE0 from the genesis :

          _____                               
    D0 ---     \______________________________------ input
          ____________________                
    D1 ---                    \_______________------ input
          ____________________                
    D2 ---                    \_______________------ input
          ____________________                
    D3 ---                    \_______________------ input
          ________                  __________
    TH ---        \________________/          ------ input
          ___________                  _______
    TL ---           \________________/       ------ output
          ____________________________________
    TR ---                                    ------ output

Notes :
 * Clock and data must not change until Ack line is switched.
 * Ack pulse can be very late. Please put a long timeout (100ms at least).
 * Genesis checks that TR is high while waiting for ack. This solves bus access
   collisions. This way, the debugger has priority over the genesis.


High level protocol
-------------------

The high level protocol uses only bytes (it never transmits an odd number of
nybbles). All 16/32 bits values are big endian (native 68k ordering).

Packets cannot grow over 40 bytes including data (that is the maximum buffer
you will ever need).

Monitor mode can be triggered externally by sending any command. Clock pin low
triggers a level 2 interrupt in the genesis, effectively making it enter the
monitor mode. The acknowledge may be a bit late in this case.


Byte level protocol
-------------------

    First byte : header
    B7..B5 : command 
      000 = handshake (ping genesis)
      001 = exit monitor mode
      010 = byte read
      011 = byte write
      100 = long read
      101 = long write
      110 = word read
      111 = word write
    B4..B0 : data size in bytes (0 means 32, size ignored if B7..B5 = 00)
    3 next bytes : address

Followed by data for write commands :

    size*bytes : data


Command-level protocol
----------------------

The genesis can be in 2 different states : normal mode and monitor mode. Normal
mode means that the genesis runs its main program. Monitor mode means that all
interrupts are disabled, all other chips are halted and the genesis waits for
incoming commands. Monitor mode is entered by sending any command to the
genesis ; to exit monitor mode, send the corresponding command when in monitor
mode. Whenever the genesis leaves monitor mode, it signals it by sending an
exit monitor mode command.

Other commands have different meanings depending on whether they are sent from
or to the genesis.

### Write commands sent to the genesis ###

This is the simplest case. Data must be sent immediatly after the command, then
the genesis writes data to its bus at the specified address using
auto-increment. Up to 32 bytes can be sent per command.

### Read commands sent to the genesis ###

The genesis will read data on its bus at the specified address using
auto-increment. It will send data back to the caller by issuing a write
command.

### Write commands issued by the genesis ###

The genesis will send write commands to upload read results.

### Read commands issued by the genesis ###

The genesis will never issue read commands.


Accessing genesis state from the debugger
-----------------------------------------

The monitor reserves a small amount of RAM for its operation. This is mapped in
bda_ram.inc. Symbols are defined to access normal mode registers
(bda_d0..bda_a7 + bda_pc + bda_sr). Their values are restored in the CPU when
leaving monitor mode.

Since these symbols may be automatically mapped by blsbuild, debugging tools
should not hardcode these symbols. The memory layout of registers will be kept
the same, so one base address is enough. The symbols bda_cpu_state and
bdp_cpu_state_end define the zone you may access for this purpose.

