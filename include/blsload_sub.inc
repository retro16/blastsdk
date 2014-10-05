BLSLOAD_START_READ macro sector, count
                CCALL2  BLSLOAD_START_READ, sector, count
                endm

BLSLOAD_READ_CD macro bytes, target
        if bytes < 512
                moveq   #bytes/2, d0
        else
                move.l  #bytes/2, d0
        endif
                lea     target, a0
                jsr     BLSLOAD_READ_CD_ASM
                endm

; vim: ts=8 sw=8 sts=8 et
