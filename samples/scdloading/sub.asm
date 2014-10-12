                include blsload_sub.inc
                include cdbios.inc
                include bdp_sub.inc

SP_INIT
                SYNC_MAIN_SUB
                rts

SP_MAIN
                sub_int_enable
         loopuntil btst #0, GA_COMMFLAGS_MAIN
         trap #7
                ;BLSLOAD_CHECK_MAIN
                blsload_start_read 0, 1         ; Read sector #0
                blsload_read_cd 4, target_data(pc) ; Read 4 bytes

                bdp_write hello, 17
         loopwhile btst #0, GA_COMMFLAGS_MAIN
                bdp_write target_data, 11

.1              ;BLSLOAD_CHECK_MAIN
                bra.b   .1                      ; Endless loop

SUB_INT_LEVEL2  
                ;move.l  l2data, target_data
                ;BLSLOAD_CHECK_MAIN
                rts

counter         dl      0
hello           db      'HELLO '
target_data     db      'DUMB'          ; Will contain 'SEGA' if successful
                db      ' WORLD\n\n'

l2data          db      'COOL'

; vim: ts=8 sw=8 sts=8 et
