        if ..UNDEF blsload_sub_inc
blsload_sub_inc set     1

BLSLOAD_CHECK_MAIN macro
                jsr     BLSLOAD_CHECK_MAIN
                endm

BLSLOAD_START_READ macro sector, count
                CCALL2  BLSLOAD_START_READ, sector, count
                endm

BLSLOAD_READ_CD macro bytes, target
                CCALL2  BLSLOAD_READ_CD, bytes, target
                endm

        endif
; vim: ts=8 sw=8 sts=8 et
