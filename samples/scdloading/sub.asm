                include blsload_sub.inc
                include cdbios.inc
                include bdp_sub.inc

SP_INIT
                rts

SP_MAIN
                SYNC_MAIN_SUB

                blsload_start_read 0, 1         ; Read sector #0
                blsload_read_cd 4, target_data(pc) ; Read 4 bytes

                bdp_write hello, 17

.1              bra.b   .1                      ; Endless loop

SUB_INT_LEVEL2  BLSLOAD_CHECK_MAIN
                rts

counter         dl      0
hello           db      'HELLO '
target_data     db      'DUMB'          ; Will contain 'SEGA' if successful
                db      ' WORLD\n\n'

; vim: ts=8 sw=8 sts=8 et
