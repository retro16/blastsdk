                include bls_init.inc
                include bls_vdp.inc
                include bdp.inc

MAIN
IP_MAIN
                bls_init 0, INT_VBLANK
                bls_init_vdp 0, 1, 64, 32, BLAST_SPLASH_PNG_MAP, 0, PLANE_B, SPRAT, HSCROLL_TABLE, 0, 0, 0, 0, 0, 0, 0

                bls_enable_interrupts
                SYNC_MAIN_SUB

                move.l  intcount, beforeload
                BLSLOAD_BINARY_BLAST_SPLASH
.1 bra.b   .1


INT_VBLANK
                addi.l  #1, intcount
                movem.l d0/d1/a0/a1, -(sp)
                SUB_INTERRUPT
                bdp_sub_check
                movem.l (sp)+, d0/d1/a0/a1
                rte

intcount        dl      0
beforeload      dl      $FFFFFFFF
afterload       dl      $FFFFFFFF

; vim: ts=8 sw=8 sts=8 et
