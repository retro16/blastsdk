BDB commands :

q                        quit
asmx <file> [addr]       Assemble a file, send it to addr and point PC at it.
subasmx <file> [addr]    Assemble a file, send it to addr of the sub CPU and point sub CPU PC at it.
flush                    flush genesis input

r <b|w|l>[len] <addr>    read len bytes/words/longs at addr. N is in bytes. If ommited, reads one.
w <b|w|l> <addr> <val>   writes val at specified address.

sr <b|w|l> <addr>        read 1 byte/word/long at addr from the sub CPU
sw <b|w|l> <addr> <val>  write 1 byte/word/long at addr from the sub CPU

sendfile <addr> <file>   writes a file in RAM
go                       resumes execution
ping                     handshake with the monitor
test                     handshake with the bridge (no data is sent to the Genesis)
reg                      display value of all registers
subreg                   display value of all sub CPU registers
dump <addr> <len> [file] dump <len> bytes of memory at <addr> (optionally writing to [file])
subdump <addr> <len> [file] dump <len> bytes of PRAM at <addr> (optionally writing to [file])
showchr <index>          show character data from VRAM in ASCII art.
vdump <addr> <len> [file] dump <len> bytes of VRAM at <addr> (optionally writing to [file])
blsgen                   run blsgen
d68k <address> <len>     disassemble memory at <address> for <len> bytes
subd68k <address> <len>  disassemble PRAM at <address> for <len> bytes
substop                  make the sub CPU enter monitor mode
subgo                    resume sub CPU execution
set <register> <value>   set the value of a register
subset <register> <value> set the value of a register in the sub CPU
step / s                 execute one instruction
skip                     skip one instruction
substep / ss             execute one instruction on the sub CPU
subskip                  skip one instruction on the sub CPU
boot <file>              boot a RAM program or a CD image (runs its IP and SP)
bootsp <file>            boot SP from a CD image
break <address>          place a breakpoint at <address>. If <address> is omitted, show a list of breakpoints.
delete <address>         delete a breakpoint at <address>
subreset                 reset sub CPU
bdpdump <0|1>            set to 1 to enable bdp low level dump

Naming conventions :

BDB (debugger) : the debugger (the interactive program running on the PC)
BDP (protocol) : the protocol and its API (sendmem, readword, ...)
BDA (agent) : protocol implementation on the main CPU - triggered by L2, TRACE and TRAP07 exceptions
bridge : the arduino program that translates USB/serial/ethernet streams from/to genesis gamepad signals.

