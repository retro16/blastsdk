                include blsload_sub.inc
                include cdbios.inc
                include bdp_sub.inc

SP_INIT
                SYNC_MAIN_SUB
                rts

SP_MAIN
                sub_int_enable

.1              
                bra.b   .1                      ; Endless loop

SUB_INT_LEVEL2  
                BLSLOAD_CHECK_MAIN
                rts

; vim: ts=8 sw=8 sts=8 et
