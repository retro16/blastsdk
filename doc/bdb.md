BDB commands :

    main                     Switch to main CPU
    sub                      Switch to sub CPU
    
    r <b|w|l> <addr>         read 1 byte/word/long at addr
    w <b|w|l> <addr> <val>   write 1 byte/word/long at addr
    
    go                       resumes execution
    stop                     stop execution
    step / s                 execute one instruction
    next / n                 execute until the next instruction (useful to skip loops/subroutines)
    skip                     skip one instruction (the instruction is not executed)
    reset                    reset the CPU
    
    reg                      display value of all registers
    dump <addr> <len> [file] dump <len> bytes of memory at <addr> (optionally writing to [file])
    showchr <index>          show character data from VRAM in ASCII art.
    vdump <addr> <len> [file] dump <len> bytes of VRAM at <addr> (optionally writing to [file])
    sendfile <addr> <file>   writes a file in RAM
    d68k <address> <len>     disassemble memory at <address> for <len> bytes
    set <register> <value>   set the value of a register
    
    boot <file>              boot a RAM program or a CD image (runs its IP and SP)
    bootsp <file>            boot only the SP from a CD image
    blsgen                   run blsgen
    asmx <file> [addr]       Assemble a file, send it to addr and point PC at it.
    
    break <address>          place a breakpoint at <address>. If <address> is omitted, show a list of breakpoints.
    delete <address>         delete a breakpoint at <address>
    sym <address>            get the symbol of an address
    print / p <sym>          print the value of a symbol
    
    q                        quit
    bridge                   get info on bridge (no data is sent to the Genesis)
    bdpdump <0|1>            set to 1 to enable bdp low level dump
    symload <file>           load the symbol table from <file> (should be build_blsgen/blsgen.md)

All commands can be prefixed by main r sub to switch between CPUs.

For example, **subreg** will switch to sub CPU and display its registers. The bdb prompt will tell if you work on the main cpu (m) or sub cpu (s).


Naming conventions :

BDB (debugger) : the debugger (the interactive program running on the PC)

BDP (protocol) : the protocol and its API (sendmem, readword, ...)

BDA (agent) : protocol implementation on the main CPU - triggered by L2, TRACE and TRAP07 exceptions

bridge : the arduino program that translates USB/serial/ethernet streams from/to genesis gamepad signals
