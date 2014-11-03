                include cdbios.inc

SP_INIT
SP_EXTRA
                BIOS_CDCSTOP
                rts

SP_MAIN
                SYNC_MAIN_SUB
                moveq   #0, d0
.loop
                addq.l  #1, d0
                move.l  d0, INDICATOR
                bra.b   .loop

                hex     FFFF FFFF
INDICATOR       dl      0
                hex     DEAD BEEF
INDICATOR2      hex     F000 0000
                hex     FFFF FFFF

SUB_INT_LEVEL2
                addi.l  #1, INDICATOR2
                rts

; vim: ts=8 sw=8 sts=8 et
