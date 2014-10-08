; void bdp_write(char *data, int length);
bdp_write       macro   data, length
                CCALL2  BDP_WRITE, data, length
                endm

                if BUS == BUS_MAIN
; Poll sub cpu state. Call this as often as possible
; void bdp_sub_check();
bdp_sub_check   macro
                if TARGET == TARGET_SCD1 || TARGET == TARGET_SCD2
                jsr     bdp_sub_check
                endif
                endm
                endif

; vim: ts=8 sw=8 sts=8 et
