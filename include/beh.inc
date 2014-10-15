        if ..UNDEF BEH_INC
BEH_INC         set     1

beh_init macro
                if TARGET == TARGET_SCD1 || TARGET == TARGET_SCD2
                jsr beh_init_scd_vectors
                endif
                endm

        endif
; vim: ts=8 sw=8 sts=8 et
