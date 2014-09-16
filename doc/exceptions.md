Blast ! SDK exception processing
================================

The SDK uses globals to define exception vectors. Globals prefixed by INT_
or HWINT_ define exception entry points and globals prefixed by INTV define
vector address.

To setup an exception vector, just declare a symbol with the correct name and
blsbuild will detect it and put its address in the ROM header. On Sega CD, a
macro named set_int_handler is provided to write exception vectors at the
correct addresses.

This is a table of all globals that blsbuild detect as exception vectors :

    Symbol         Vector   Description
    INTERRUPT    xx       Default interrupt vector (for undefined exceptions)
    INT_BUSERR   08       Bus error
    INT_ADDRERR  0C       Illegal address
    INT_ILL      10       Illegal instruction
    INT_ZDIV     14       Divide by zero
    INT_CHK      18       CHK error
    INT_TRAPV    1C       TRAPV exception
    INT_PRIV     20       Privilege violation
    INT_TRACE    24       Trace instruction (used by BDA)
    INT_LINEA    28       Line A interrupt
    INT_LINEF    2C       Line F interrupt
    INT_SPURIOUS 60       Spurious interrupt
    INT_LEVEL1   64       Level 1 interrupt
    INT_LEVEL2   68       Level 2 interrupt
    INT_PAD      68       Level 2 interrupt (gamepad interrupt, used by BDA)
    INT_LEVEL3   6C       Level 3 interrupt
    INT_LEVEL4   70       Level 4 interrupt
    INT_HBLANK   70       Level 4 interrupt (horizontal blanking)
    INT_LEVEL5   74       Level 5 interrupt
    INT_LEVEL6   78       Level 6 interrupt
    INT_VBLANK   78       Level 6 interrupt (vertical blanking)
    INT_LEVEL7   7C       Level 7 interrupt
    
    INT_TRAP00   80       TRAP #00
    [...]
    INT_TRAP07   9C       TRAP #07 (used by BDA for breakpoints)
    [...]
    INT_TRAP08   A0       TRAP #08
    INT_TRAP15   BC       TRAP #15
