SP_INIT
                SYNC_MAIN_SUB
                rts

SP_MAIN
                moveq   #0, d0
.loop
                addq.l  #1, d0
                move.l  d0, INDICATOR.w
                bra.b   .loop

INDICATOR       dl      0
                hex     ABCD

SUB_INT_LEVEL2
                rts

; vim: ts=8 sw=8 sts=8 et
