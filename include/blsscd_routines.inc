BLSCDR_COMM     equ     (GA_COMMWORDS + $10 - 4)        ; Last 2 main CPU commwords used for CDR
BLSCDR          equ     $20
BLSCDR_BIT      equ     5


        if BUS == BUS_MAIN

BLSLOAD_START_READ macro sector, count

                ; Main CPU load prepare
                move.l  #(sector | (count << 24)), BLSCDR_COMM
                bset    #BLSCDR_BIT, GA_COMMFLAGS_MAIN

        if TARGET == TARGET_SCD1
                ; Swap request
                bclr    #GA_DMNA_BIT, GA_MM + 1
        else
                assert  TARGET == TARGET_SCD2

                lea     GA_MM + 1, a0

                ; Check if WRAM is already available for sub cpu
                btst    #GA_RET_BIT, (a0)
                beq.b   .bank_available\?

                ; Send 2M bank to sub CPU
                bset    #GA_DMNA_BIT, (a0)
                loopuntil btst #GA_DMNA_BIT, (a0)
.bank_available\?
        endif

                endm

; Wait until data is ready and start transfer
BLSLOAD_WAIT_READ macro
        if TARGET == TARGET_SCD1

                ; Wait until swap finished
                loopuntil btst #GA_DMNA_BIT, GA_MM + 1

        else
                assert  TARGET == TARGET_SCD2

                ; Wait until memory returned to main cpu
                loopuntil btst #GA_RET_BIT, GA_MM + 1

        endif
                bclr    #BLSCDR_BIT, GA_COMMFLAGS_MAIN
                endm
        

; Test if BLSLOAD_WAIT_READ would block
; Flag Z is set if it would block
; Flag Z is clear if data is available
BLSLOAD_READ_READY macro
        if TARGET == TARGET_SCD1
                btst #GA_DMNA_BIT, GA_MM + 1
        else
                assert TARGET == TARGET_SCD2
                btst #GA_RET_BIT, GA_MM + 1
        endif
                endm

; Read <count> sectors starting at <sector> to word RAM
; Blocks until word RAM is reachable
BLSLOAD_READ_CD macro sector, count
                BLSLOAD_START_READ sector, count
                BLSLOAD_WAIT_READ
                endm

        endif   ; BUS == BUS_MAIN

; vim: ts=8 sw=8 sts=8 et
