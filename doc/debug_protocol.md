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

    Pin Bit Ard Name Monitor  Pad0  Pad1 Serial
     1   0    4   D0      D0    Up    Up      -
     2   1    5   D1      D1  Down  Down      -
     3   2    6   D2      D2     0  Left      -
     4   3    7   D3      D3     0 Right      -
     5   -   NC  Vcc       -     -     -      -
     6   4    8   TL     Ack     A     B     Tx
     7   6    3   TH   Clock   Sel   Sel    INT
     8   -  GND  GND       -     -     -      -
     9   5    9   TR  Busreq Start     C     Rx
    NC   7    -   -        -    ?      ?      -

Bit is the control bit of the port register in the genesis.

Ard is the Arduino pin. To remap pins, you must modify the Arduino source code.

Pad0 is the value returned by the gamepad when Sel is low, Pad1 is the state
when Sel is high. Serial is the pin role when the port is in serial mode
(unused in blast debugger).


Byte transmission format
------------------------

The debug mode communicates via D-Pad data lines, passing 4 bits per clock.  TH
is the clock, read on edge. Format is big nybble first. For example, to pass
0xA3, digit A would be passed first, then 3. Each nybble is acknowledged by a
TL transition. 5V pull-up is logic high and 0V is logic low.

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
 * input/output is the pin state of the arduino.
 * Clock and data must not change until Ack line is switched.
 * Ack pulse can be very late. Please put a long timeout (100ms at least).
 * When the genesis sends data, it checks TR while waiting for acknowledge.
   This way, if the debugger sends data while the genesis is transmitting,
   the genesis immediatly stops and enters receive mode. This solves bus access
   conflicts and can help waking up the genesis from a blocked state.


High level protocol
-------------------

The high level protocol uses only bytes (it never transmits an odd number of
nybbles). All 16/32 bits values are big endian (native 68k ordering).

Packets cannot grow over 36 bytes including data (that is the maximum buffer
you will ever need for one command).

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
    B4..B0 : data size in *bytes* (0 means 32, no data if B7..B6 = 00)
    3 next bytes : address

Followed by data for write commands :

    size*bytes : data

Note : the data size field always indicates the size in *bytes*, even if you
send words or longs. Sending an odd number of bytes during word transfers leads
to undefined behaviour.


Command-level protocol
----------------------

The genesis can be in 2 different states : normal mode and monitor mode. Normal
mode means that the genesis runs its main program. Monitor mode means that all
interrupts are disabled and the genesis waits for incoming commands. Monitor
mode is entered by sending any command to the genesis ; to exit monitor mode,
send the corresponding command when in monitor mode. Whenever the genesis
leaves monitor mode, it sends an exit monitor mode command.

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

### Examples ###

Set word at 0xFF0020 to 0xCAFE and word at 0xFF0022 to 0xBABE

    E4 FF 00 20 CA FE BA BE

Exit monitor mode

    20 00 00 00

Read long word at 0x000200 (first line is sent to the genesis, second line is
received from the genesis)

    84 00 02 00
    A4 00 02 00 53 45 47 41


Accessing genesis CPU state from the monitor
--------------------------------------------

On the genesis, the last 74 bytes of RAM contain CPU registers, updated when
entering and leaving the monitor. Since these registers are read/write, you can
read and alter CPU registers directly in monitor mode by accessing the correct
addresses. This includes SR to do, for example, step by step execution using
TRACE mode of the 68000.

Mapping is the following :

    FFFFBA : D0..D7
    FFFFDA : A0..A7
    FFFFFA : PC
    FFFFFE : SR


Catching interrupts
-------------------

The Blast ! Debugger Agent (BDA) catches TRAP 7 and TRACE interrupts. When such
interrupts occur, the genesis sends a handshake packet with the address field
set to the exception vector and enters monitor mode :

Data sent by the genesis when TRACE triggered :

    00 00 00 09


Data sent by the genesis when TRAP 7 triggered :

    00 00 00 27

Only these 2 interrupts are caught by default. You must ensure that the
interrupt vectors are set to g_int_trace and g_int_trap07 of bda.inc to make
it work correctly.

