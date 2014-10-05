                include bls_init_sub.inc
                include blsload_sub.inc
SP_MAIN
                bls_init_sub
                bls_enable_interrupts

                SYNC_MAIN_SUB

                BLSLOAD_START_READ 0, 1         ; Read first sector
                BLSLOAD_READ_CD 4, target_data(pc) ; Read 4 bytes

.1              bra.b        .1                 ; Endless loop

counter         dl      0
                db      'HELLO '
target_data     ds      4               ; Will contain 'SEGA' if successful
                db      ' WORLD'

; vim: ts=8 sw=8 sts=8 et
