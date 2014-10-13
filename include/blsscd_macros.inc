
SYNC_MAIN_SUB   macro

        if BUS == BUS_MAIN

                lea GA_COMMFLAGS + 1, a0        ; Write to -1(a0), read from (a0)
                move.b  (a0), d0
                andi.b  #$10, d0
.waitsub\?
                
                ; Compare bit 4 of d0 with bit 4 of (a0)
                move.b  (a0), d1
                andi.b  #$10, d1
                eor.b   d0, d1
                beq.b   .waitsub\?              ; Jump if still equal

                bchg    #4, -1(a0)              ; Stop sub oscillation by flipping the bit once

        else

                lea GA_COMMFLAGS, a0            ; Write to 1(a0), read from (a0)
                move.b  (a0), d0
                andi.b  #$10, d0
.waitmain\?
                bchg    #4, 1(a0)               ; Oscillate
                
                ; Compare bit 4 of d0 with bit 4 of (a0)
                move.b  (a0), d1
                andi.b  #$10, d1
                eor.b   d0, d1
                beq.b   .waitmain\?             ; Jump if still equal

        endif

                endm

        if BUS == BUS_MAIN

SUB_BUSREQ      macro
                move.b  #(GA_NORESET | GA_BUSREQ), GA_RH + 1
.1\?            btst.b  #GA_BUSREQ_BIT, GA_RH + 1       ; Wait until bus is ready
                beq.b   .1\?
                endm

SUB_BUSRELEASE  macro
                move.b  #GA_NORESET, GA_RH + 1
                endm

SUB_RESET       macro
                move.b  #GA_NORESET, GA_RH + 1
                move.b  #0, GA_RH + 1
                move.b  #GA_NORESET, GA_RH + 1
                endm

SUB_SET_BANK    macro   bank
                move.b  GA_MM + 1, d0
                lsr.b   #1, d0          ; MODE flag represents neutral operation for DMNA
                andi.b  #$02, d0        ; Clear other bits
                ori.b   #(bank << 6), d0
                move.b  d0, GA_MM + 1   ; Set bank without affecting WRAM
                endm

SUB_ACCESS_PRAM macro bank
                lea     GA_RH + 1, a0
                move.b  (a0), d0
                move.b  d0, d1          ; Store original state of GA_RH + 1
                bset    #GA_BUSREQ_BIT, d0
                move.b  d0, (a0)
.1\?            cmp.b   (a0), d0
                bne.b   .1\?
                swap    d1              ; Use second part of d1
                addq    #2, a0  ; Point a0 at GH_MM + 1
                move.b  (a0), d0
                move.b  d0, d1          ; Store original state of GH_MM + 1
                move.l  d1, -(a7)       ; Push current state
                lsr.b   #1, d0          ; MODE flag represents neutral operation for DMNA
                andi.b  #$02, d0        ; Clear other bits
                ori.b   #(bank << 6), d0
                move.b  d0, (a0)
                endm

SUB_RELEASE_PRAM macro
                move.l  (a7)+, d0
                swap    d0
                move.b  d0, GA_RH + 1           ; Restore old busreq status
                swap    d0
                
                ; Restore old bank setting
                move.b  d0, d1
                lsr.b   #1, d1
                andi.b  #$02, d1
                andi.b  #$C0, d0
                or.b    d1, d0
                move.b  d0, GA_MM + 1
                endm

        endif

SUB_INTERRUPT   macro
                bset    #0, GA_RH
                endm

SUB_INT_ENABLE  macro
                move    #$2000, SR
                move.b  #GA_IM_L1|GA_IM_L2|GA_IM_L3|GA_IM_L4|GA_IM_L5|GA_IM_L6, GA_IMASK + 1
                endm

SUB_INT_DISABLE macro
                move    #$2700, SR
                move.b  #0, GA_IMASK + 1
                endm

SET_INT_HANDLER macro   vector, target
                move.l  vector, a0
                move.w  #$4EF9, (a0)+
                move.l  #target, (a0)
                endm

SEND_WRAM_2M    macro
        if BUS == BUS_MAIN
                bset    #GA_DMNA_BIT, GA_MM + 1
                loopuntil btst #GA_DMNA_BIT, GA_MM + 1
        else
                assert BUS == BUS_SUB
                bset    #GA_RET_BIT, GA_MM + 1
                loopwhile btst #GA_RET_BIT, GA_MM + 1
        endif
                endm

                ; Blocks until wram is available
WAIT_WRAM_2M    macro
        if BUS == BUS_MAIN
                loopuntil btst #GA_RET_BIT, GA_MM + 1
        else
                assert BUS == BUS_SUB
                loopwhile btst #GA_RET_BIT, GA_MM + 1
        endif
                endm


                ; Sets Z flag if WRAM is unavailable
                ; Clears Z flag if WRAM is available
HAS_WRAM_2M     macro
                btst    #GA_RET_BIT, GA_MM + 1
        if BUS == BUS_SUB
                eori    #$04, CCR               ; Reverse the condition
        endif
                endm

        if BUS == BUS_SUB
SWAP_WRAM_1M    macro
                bchg    #GA_RET_BIT, GA_MM + 1
                loopwhile btst #GA_DMNA_BIT, GA_MM + 1
                endm

                ; This must be called on both CPUs:
                ; the two CPUs wait for each other, then WRAM is swapped
SWAP_SYNC_1M    macro
                loopuntil btst #GA_DMNA_BIT, GA_MM + 1
                SWAP_WRAM_1M
                endm
        else
                assert BUS == BUS_MAIN

                ; This must be called on both CPUs:
                ; the two CPUs wait for each other, then WRAM is swapped
SWAP_SYNC_1M    macro
                bset    #GA_DMNA_BIT, GA_MM + 1
                loopwhile btst #GA_DMNA_BIT, GA_MM + 1
                endm

                ; Send a swap request
SWAP_REQ_1M     macro
                bset    #GA_DMNA_BIT, GA_MM + 1
                endm

                ; Wait until swap request has been honored by the sub CPU
SWAP_WAIT_1M    macro
                loopwhile btst #GA_DMNA_BIT, GA_MM + 1
                endm
        endif

; vim: ts=8 sw=8 sts=8 et

