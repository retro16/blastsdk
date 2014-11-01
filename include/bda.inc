        if BUS == BUS_MAIN

         if ..UNDEF BLS_NBDA
bda_enabled     set     1
         endif
bda_init        macro
         if ..UNDEF BLS_NBDA
                jsr     bda_init
         endif
                endm
        endif

; vim: ts=8 sw=8 sts=8 et
