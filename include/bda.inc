                if BUS == BUS_MAIN
bda_enabled     set     1
bda_init        macro
                jsr     bda_init
                endm
                endif

; vim: ts=8 sw=8 sts=8 et
