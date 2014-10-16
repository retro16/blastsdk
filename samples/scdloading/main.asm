                include bls_init.inc
                include bls_vdp.inc
                include bdp.inc

MAIN
IP_MAIN
                bls_init 0, INT_VBLANK
                bls_init_vdp 0, 1, 64, 64, $C000, 0, $E000, $B000, $BE00, 0, 0, 0, 0, 0, 0, 0

                bls_enable_interrupts
                SYNC_MAIN_SUB

.1              
              VDPSETBG #$600
              VDPSETBG #$404
                btst    #CBTNSTART_BIT, CDATA1
                bne.b   .1


                BLSLOAD_BINARY_MAIN2
                jmp     MAIN2

INT_VBLANK
                movem.l d0/d1/a0/a1, -(sp)
                SUB_INTERRUPT
                bdp_sub_check
                movem.l (sp)+, d0/d1/a0/a1
                rte

; vim: ts=8 sw=8 sts=8 et
