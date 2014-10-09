memcpy  macro   dest, source, size
        if BUS == BUS_MAIN
        CCALL3  _MAIN_MEMCPY, dest, source, size
        elsif BUS == BUS_SUB
        CCALL3  _SUB_MEMCPY, dest, source, size
        endif
        endm

memset  macro   dest, source
        if BUS == BUS_MAIN
        CCALL3  _MAIN_MEMSET, dest, source, size
        elsif BUS == BUS_SUB
        CCALL3  _SUB_MEMSET, dest, source, size
        endif
        endm

; vim: ts=8 sw=8 sts=8 et
