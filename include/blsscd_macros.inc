
SYNC_MAIN_SUB   macro

        if BUS == BUS_MAIN

        lea GA_COMMFLAGS + 1, a0
        bset    #4, -1(a0)
.1m\?
        btst    #4, (a0)
        beq.b   .1m\?
        bclr    #4, -1(a0)
.2m\?
        btst    #4, (a0)
        bne.b   .2m\?

        else

        lea GA_COMMFLAGS, a0
        bset    #4, 1(a0)
.1s\?
        btst    #4, (a0)
        beq.b   .1s\?
        bclr    #4, 1(a0)
.2s\?
        btst    #4, (a0)
        bne.b   .2s\?

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
                move.b  (a0), -(a7)     ; Push current busreq status on stack
                bset    #GA_BUSREQ_BIT, (a0)
.1\?    btst    #GA_BUSREQ_BIT, (a0)
                beq.b   .1\?
                
                addq    #2, a0  ; Point a0 at GH_MM + 1
                move.b  (a0), d0
                move.b  d0, -(a7)       ; Push current bank on stack
                lsr.b   #1, d0          ; MODE flag represents neutral operation for DMNA
                andi.b  #$02, d0        ; Clear other bits
                ori.b   #(bank << 6), d0
                move.b  d0, (a0)
                endm

SUB_RELEASE_PRAM        macro
                move.b  (a7)+, d0
                move.b  (a7)+, GA_RH + 1        ; Restore old busreq status
                
                ; Restore old bank setting
                move.b  d0, d1
                lsr.b   #1, d1
                andi.b  #$02, d1
                andi.b  #$C0, d0
                ori.b   d1, d0
                move.b  d0, GA_MM + 1
                endm

        endif

SUB_INTERRUPT   macro
                bset    #0, GA_RH
                endm

SUB_INT_ENABLE  macro
                move.b  #GA_IM_L1|GA_IM_L2|GA_IM_L3|GA_IM_L4|GA_IM_L5|GA_IM_L6, GA_IMASK + 1
                move    #$2000, SR
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
                bset    #GA_DMNA_BIT, GA_MM + 1
                loopuntil btst #GA_DMNA_BIT, GA_MM + 1
                endm

                if BUS == BUS_SUB
SWAP_WRAM_1M    macro
                bchg    #GA_RET_BIT, GA_MM + 1
                loopwhile btst #GA_DMNA_BIT, GA_MM + 1
                endm
                endif

                include blsscd_read_macros.inc

; vim: ts=8 sw=8 sts=8 et

