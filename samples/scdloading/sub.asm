                include blsload_sub.inc
                include cdbios.inc
                include bdp.inc

SP_INIT
                rts

SP_MAIN
                SYNC_MAIN_SUB
.0              bra.b   .0
                trap    #7
.2              bra.b   .2
                BDP_WRITE hello, 5

                ;BLSLOAD_START_READ 2, 1         ; Read sector #2
                ;BLSLOAD_READ_CD 4, target_data(pc) ; Read 4 bytes

.1              bra.b   .1                      ; Endless loop

counter         dl      0
hello           db      'HELLO '
target_data     ds      4               ; Will contain 'SEGA' if successful
                db      ' WORLD'
debug_out       ds      100

; vim: ts=8 sw=8 sts=8 et
