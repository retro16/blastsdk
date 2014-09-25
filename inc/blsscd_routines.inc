BLSCDR_COMM     equ     (GA_COMMWORDS + $10 - 4)        ; Last 2 main CPU commwords used for CDR
BLSCDR          equ     $20
BLSCDR_BIT      equ     5


BLSLOAD_PREPARE macro   sector, count
        if BUS == BUS_MAIN

        ; Main CPU load prepare
        move.l  #(sector | (count << 24)), BLSCDR_COMM
        bset    #BLSCDR_BIT, GA_COMMFLAGS_MAIN
        SUB_INTERRUPT

        else

        ; Sub CPU load prepare
subload\?
        move.l  #sector, BLSLOAD_SECTOR
        move.w  #count, BLSLOAD_COUNT + 2
        lea     BLSLOAD_SECTOR, a0
        BIOS_ROMREADN

        endif
        endm

; Wait until data is ready and start transfer
BLSLOAD_START           macro
        lea     GA_MM + 1, a0
        if BUS == BUS_MAIN

        if TARGET == TARGET_SCD1

        ; Swap request
        bclr    #GA_DMNA_BIT, (a0)
BLSLOAD_WAIT_START_MODE_1\?
        btst    #GA_DMNA_BIT, (a0)         ; Wait until swap finished
        beq.b   BLSLOAD_WAIT_START_MODE_1\?

        else
        assert  TARGET == TARGET_SCD2

        ; Check if WRAM is already available for sub cpu
        btst    #GA_RET_BIT, (a0)
        beq.b   BLSLOAD_WAIT_START_MODE_2\?

        ; Send 2M bank to sub CPU
        bset    #GA_DMNA_BIT, (a0)
        loopuntil btst #GA_DMNA_BIT, (a0)

BLSLOAD_WAIT_START_MODE_2\?
        ; Wait until memory returned to main cpu
        loopuntil btst #GA_RET_BIT, (a0)

        endif
        
BLSLOAD_READ_READY\?
        bclr    #BLSCDR_BIT, GA_COMMFLAGS_MAIN
        move.l  a2, -(a7)
        lea     $200000, a2

        else
        ; Nothing to do to prepare transfert
        endif
        endm

; Stream <size> bytes from CD to <addr>. <size> must be even and addr must be word aligned.
BLSLOAD_READ    macro   addr, size
        if BUS == BUS_SUB

        assert  (size & 1) == 0
        move.l  #(size / 2), d0
        lea     addr, a0
        jsr     BLSLOAD_READ_CD

        endif
        endm


; Call this to stop stream
BLSLOAD_FINISH  macro
        if BUS == BUS_MAIN
        move.l  (a7)+, a2       ; Restore data source register
        else

        ; Ack the last sector if needed
        tst.w   BLSLOAD_BUFOFF
        bne.b   BLSLOAD_FINISH_ACK\?
        BIOS_CDCACK
        clr.w   BLSLOAD_BUFOFF

        endif
        endm

; vim: ts=8 sw=8 sts=8 et
