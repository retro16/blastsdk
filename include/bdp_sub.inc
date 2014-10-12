; void bdp_write(char *data, int length);
bdp_write       macro   data, length
        if BUS == BUS_MAIN
                CCALL2  _MAIN_BDP_WRITE, data, length
        elsif BUS == BUS_SUB
                CCALL2  _SUB_BDP_WRITE, data, length
        endif
                endm

        if BUS == BUS_MAIN
; Poll sub cpu state. Call this as often as possible
; void bdp_sub_check();
bdp_sub_check   macro
        if TARGET == TARGET_SCD1 || TARGET == TARGET_SCD2
                jsr     BDP_SUB_CHECK
        endif
                endm
        endif

; vim: ts=8 sw=8 sts=8 et
