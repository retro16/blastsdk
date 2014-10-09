                include bls_init.inc
                include bls_vdp.inc
                include bdp.inc

MAIN
IP_MAIN
                bls_init 0, INT_VBLANK
                bls_enable_interrupts

                bls_init_vdp 0, 1, 64, 64, $C000, 0, $E000, $B000, $BE00, 0, 0, 0, 0, 0, 0, 0

                SYNC_MAIN_SUB

                move.w  #1, ready
CPUSYNC
.1              bra.b .1

ready           dw      0

INT_VBLANK      movem.l d0/d1/a0/a1, -(sp)
                lea     bgcolor(pc), a0
                tst.w   ready
                beq.b   .notready
                addi.w  #$020, (a0)
                andi.w  #$EEE, (a0)
                VDPSETBG (a0)
.notready
                SUB_INTERRUPT
                bdp_sub_check
                movem.l (sp)+, d0/d1/a0/a1
                rte

bgcolor         dw      $040

; vim: ts=8 sw=8 sts=8 et
