BLSLOAD_START_READ macro sector, count
                CCALL2  BLSLOAD_START_READ, sector, count
                endm

BLSLOAD_READ_CD macro bytes, target
                CCALL2  BLSLOAD_READ_CD, bytes, target
                endm

; vim: ts=8 sw=8 sts=8 et
