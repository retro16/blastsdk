        if ..UNDEF BDP_SUB_INC
BDP_SUB_INC     set     1

; void bdp_write(char *data, int length);
bdp_write       macro   data, length
                CCALL2  _SUB_BDP_WRITE, data, length
                endm

        endif
; vim: ts=8 sw=8 sts=8 et
