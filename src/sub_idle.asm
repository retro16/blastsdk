        if TARGET == TARGET_SCD1 || TARGET == TARGET_SCD2

                include blsload_sub.inc
                include cdbios.inc
                include bdp_sub.inc

SP_INIT
                set_wram_default_mode
                sync_main_sub
                rts

SP_MAIN
                sub_int_enable

.1              
                bra.b   .1                      ; Endless loop

SUB_INT_LEVEL2  
                blsload_check_main
                rts

        endif

; vim: ts=8 sw=8 sts=8 et
