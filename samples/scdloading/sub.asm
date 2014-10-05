                include bls_init_sub.inc
                include blsload_sub.inc
SP_MAIN
                addi.l  #1, counter
                bls_init_sub
                addi.l  #1, counter
                bls_enable_interrupts
.2
                addi.l  #1, counter
                bra.b   .2

                ;BLSLOAD_START_READ 0, 1         ; Read first sector
                ;BLSLOAD_READ_CD 4, target_data(pc) ; Read 4 bytes

.1              bra.b        .1                 ; Endless loop

counter         dl      0
                db      'HELLO '
target_data     ds      4               ; Will contain 'SEGA' if successful
                db      ' WORLD'

; vim: ts=8 sw=8 sts=8 et
