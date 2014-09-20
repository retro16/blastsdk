The Blast ! Debugger protocol v2
================================

Electrical specification.
-------------------------

The genesis uses 5V logic : 5V for logic high and 0V for logic low.
The genesis has an internal pull-up on input pins, and output pins
are regular outputs (not open collector).

Port mapping / pinout

Looking at the console socket :

     -----------
    \ 1 2 3 4 5 /
     \ 6 7 8 9 /
       -------

    Pin Bit Ard Name Mon.Out  Mon.In  Pad0  Pad1 Serial
     1   0    4   D0      D0      D0    Up    Up      -
     2   1    5   D1      D1      D1  Down  Down      -
     3   2    6   D2      D2      D2     0  Left      -
     4   3    7   D3      D3      D3     0 Right      -
     5   -   NC  Vcc       -       -     -     -      -
     6   4    8   TL  Clk/In  Clk/In     A     B     Tx
     7   6    3   TH Clk/Out Clk/Out   Sel   Sel    INT
     8   -  GND  GND       -       -     -     -      -
     9   5    9   TR  Mode:1  Mode:0 Start     C     Rx
    NC   7    -   -        -       -    ?      ?      -

Bit is the control bit of the port register in the genesis.

Ard is the Arduino pin. To remap pins, you must modify the Arduino source code.

Pad0 is the value returned by the gamepad when Sel is low, Pad1 is the state
when Sel is high. Serial is the pin role when the port is in serial mode
(unused in blast debugger).

TR pin indicates communication direction : If high, genesis sends data to
the arduino ; if TR is low, genesis is waiting for incoming data. This pin also
indicates whether the arduino is allowed to set D0..D3 as outputs or not.

When TR is high (genesis sends data), TL is the clock and TH is the ack pulse.
When TR is low (arduino sends data), TH is the clock and TL is the ack pulse.

When transmission is inactive, TH must be set to pull-up input in case the
genesis polls the gamepad on that port, which could cause a short-circuit.

Byte transmission format
------------------------

The debug mode communicates via D-Pad data lines, passing 4 bits per clock.
Format is big nybble first. For example, to pass 0xA3, digit A would be passed
first, then 3. Each nybble is acknowledged by a transition. 5V pull-up is logic
high (bit set) and 0V is logic low (bit clear). When unused, pins should be high.

Example transmitting 0xA4 to the genesis :

    D0..D3       A               4
          _                               ____
    D0 --- \_____________________________/    ------ output
          ________________                ____
    D1 ---                \______________/    ------ output
          _                ___________________
    D2 --- \______________/                   ------ output
          ________________                ____
    D3 ---                \______________/    ------ output
          ______                 _____________
    TH ---      \_______________/             ------ output
          ___________                 ________
    TL ---           \_______________/        ------ input

    TR ---____________________________________------ input


Example receiving 0xE0 from the genesis :

    D0..D3         E             0
          ____                            ____
    D0 ---    \__________________________/    ------ input
          ________________                ____
    D1 ---                \______________/    ------ input
          ________________                ____
    D2 ---                \______________/    ------ input
          ________________                ____
    D3 ---                \______________/    ------ input
          ___________                  _______
    TH ---           \________________/       ------ output
          ________               _____________
    TL ---        \_____________/             ------ input
             ______________________________
    TR ---__/                              \__------ input

Notes :
 * input/output is the pin state of the arduino.
 * Clock and data must not change until Ack line is switched.
 * First ack pulse can be very late. Please put a long timeout (100ms at least).
 * Second ack pulse (low to high) is always very quick.
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


Testing intermediate connectivity
---------------------------------

A special handshake allows to test communication between the PC and the
Arduino. This command never reaches the Genesis. It allows to query version
and revision of the bridge. This packet has

Send a version request :

    20 56 51 0A    " VQ\n"

The Arduino board will reply with a version and revision number

    20 32 31 30    " 210" (protocol v2, implementation v1.0)

The current arduino board also supports connectivity tests from the genesis
side (the genesis can send 2056510A), but no genesis code make use of it.

If you enabled LED in the arduino code, this test also blinks the LED, so it
may be a bit slow to respond.


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
interrupt vectors are set to int_trace and int_trap07 of bda.inc to make
it work correctly.

If these interrupts happen on the sub cpu, the following data is sent :

Data sent by the genesis when TRACE triggered on the sub cpu :

    00 00 01 09

Data sent by the genesis when TRAP 7 triggered on the sub cpu :

    00 00 01 27



Receiving arbitrary data from the Genesis
-----------------------------------------

The Genesis may send small packets of arbitrary data (1-3 bytes per command) :
The 3 most significant bits of header are 0 (like a pong), the 5 remaining bits
specify the data length (1 to 3) and data is in the address field.

Packet sent by the genesis to send "\n" (0A) :

    01 0A 00 00

Packet sent by the genesis to send "OK\n" (4F4B0A) :

    03 4F 4B 0A

To send more than 3 bytes, you must send as many commands as needed. This is
inefficient, but it avoids doing complex computations when implementing the
low level protocol in the Arduino

On the sega cd, data may be sent from both cpu. There is no way to tell which cpu sent data.


Flow control in serial mode
---------------------------

Since most arduino boards have no flow control, an acknowledge byte is sent
after each packet successfully sent to the genesis.

The acknowledge is a single byte with the value : 0x3F. It is sent by the
arduino board, NOT by the genesis.

The PC should not send another packet before receiving the ack.

Example reading a long word at 0x000200 on the serial port :

    84 00 02 00
    3F A4 00 02 00 53 45 47 41

This is what is seen on the genesis side :

    84 00 02 00
    A4 00 02 00 53 45 47 41


Main-sub cpu communication
--------------------------

The sub cpu cannot access the pad port, nor it can interrupt the main cpu. Communication between the sub cpu and the debugger is
done by setting communication flags.

Communication flags (gate array + $0E) :

    |------------ MAIN -------------|------------- SUB -------------|
    | 15| 14| 13| 12| 11| 10| 09| 08| 07| 06| 05| 04| 03| 02| 01| 00|
    |REQ|   |CDR|SYN|   |   |   |   |MON|TRA|BUF|SYN|   |   |   |   |
    |-------------------------------|-------------------------------|

Main -> sub flags :

 - REQ: The main cpu requests the sub cpu to enter monitor mode. This flag is checked by the sub cpu in its level 2 exception routine.

 - CDR: CD read request. commwords 14-15 contain sector, commword 13 contains sector count.
   The sub cpu will read the specified sectors from cd and put them at the beginning of word ram. The sub cpu will release word ram once data is ready. This mechanism is used by blsgen binary loader.

 - SYN: main-sub synchronization

Sub -> main flags :

 - MON: The sub cpu is in monitor mode

 - TRA: The sub cpu issued a TRACE exception

 - BUF: The sub cpu communication buffer contains data

 - SYN: main-sub synchronization


To output data (bdp_write) from the sub cpu, the routine places data length at $30 and data in $32-$60 range in the program ram, sets BUF flag and waits until data length at $30 is reset to 0 by the main cpu.
