        if ..UNDEF BEH_INC
        if ..UNDEF BLS_NBEH
BEH_INC         set     1
        endif

beh_init macro
        if ..UNDEF BLS_NBEH
        if TARGET == TARGET_SCD1 || TARGET == TARGET_SCD2
                jsr beh_init_scd_vectors
        endif
        endif
        endm

        endif
; vim: ts=8 sw=8 sts=8 et
