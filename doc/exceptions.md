Blast ! SDK exception processing
================================

The SDK uses globals to define exception vectors. Globals prefixed by G_INT_
or G_HWINT_ define exception entry points and globals prefixed by G_INTV
define vector address.

To setup an exception vector, just declare a symbol with the correct name and
blsbuild will detect it and put its address in the ROM header. On Sega CD, a
macro named bls_setvectors is provided to write exception vectors at the
correct addresses.

This is a table of all globals that blsbuild detect as exception vectors :

    Symbol         Vector   Description
    G_INTERRUPT    xx       Default interrupt vector (for undefined exceptions)
    G_INT_BUSERR   08       Bus error
    G_INT_ADDRERR  0C       Illegal address
    G_INT_ILL      10       Illegal instruction
    G_INT_ZDIV     14       Divide by zero
    G_INT_CHK      18       CHK error
    G_INT_TRAPV    1C       TRAPV exception
    G_INT_PRIV     20       Privilege violation
    G_INT_TRACE    24       Trace instruction (used by BDA)
    G_INT_LINEA    28       Line A interrupt
    G_INT_LINEF    2C       Line F interrupt
    G_INT_SPURIOUS 60       Spurious interrupt
    G_INT_LEVEL1   64       Level 1 interrupt
    G_INT_LEVEL2   68       Level 2 interrupt
    G_INT_PAD      68       Level 2 interrupt (gamepad interrupt, used by BDA)
    G_INT_LEVEL3   6C       Level 3 interrupt
    G_INT_LEVEL4   70       Level 4 interrupt
    G_INT_HBLANK   70       Level 4 interrupt (horizontal blanking)
    G_INT_LEVEL5   74       Level 5 interrupt
    G_INT_LEVEL6   78       Level 6 interrupt
    G_INT_VBLANK   78       Level 6 interrupt (vertical blanking)
    G_INT_LEVEL7   7C       Level 7 interrupt
    
    G_INT_TRAP00   80       TRAP #00
    [...]
    G_INT_TRAP07   9C       TRAP #07 (used by BDA for breakpoints)
    [...]
    G_INT_TRAP08   A0       TRAP #08
    G_INT_TRAP15   BC       TRAP #15
