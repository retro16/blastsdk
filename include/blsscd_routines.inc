BLSCDR_COMM     equ     (GA_COMMWORDS + $10 - 4)        ; Last 2 main CPU commwords used for CDR
BLSCDR          equ     $20
BLSCDR_BIT      equ     5


        if BUS == BUS_MAIN

BLSLOAD_START_READ macro   sector, count

                ; Main CPU load prepare
                move.l  #(sector | (count << 24)), BLSCDR_COMM
                bset    #BLSCDR_BIT, GA_COMMFLAGS_MAIN
                SUB_INTERRUPT

                endm

; Wait until data is ready and start transfer
BLSLOAD_START   macro
                lea     GA_MM + 1, a0

        if TARGET == TARGET_SCD1

                ; Swap request
                bclr    #GA_DMNA_BIT, (a0)

                ; Wait until swap finished
                loopuntil btst #GA_DMNA_BIT, (a0)

        else
                assert  TARGET == TARGET_SCD2

                ; Check if WRAM is already available for sub cpu
                btst    #GA_RET_BIT, (a0)
                beq.b   .BLSLOAD_WAIT_START_MODE_2\?

                ; Send 2M bank to sub CPU
                bset    #GA_DMNA_BIT, (a0)
                loopuntil btst #GA_DMNA_BIT, (a0)

.BLSLOAD_WAIT_START_MODE_2\?
                ; Wait until memory returned to main cpu
                loopuntil btst #GA_RET_BIT, (a0)

        endif
                endm
        
        endif   ; BUS == BUS_MAIN

; vim: ts=8 sw=8 sts=8 et
