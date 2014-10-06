                include bls_init_sub.inc
                include blsload_sub.inc
                include cdbios.inc

SP_MAIN
                bls_init_sub
                bls_enable_interrupts

                ;moveq   #1, d1
                ;moveq   #0, d0
                ;lea     $6800, a0
                ;jsr     ReadCD

                BLSLOAD_START_READ 0, 1         ; Read first sector
                BLSLOAD_READ_CD 4, target_data(pc) ; Read 4 bytes
                BLSLOAD_READ_CD 4, $7000

.1              bra.b   .1                      ; Endless loop

counter         dl      0
                db      'HELLO '
target_data     ds      4               ; Will contain 'SEGA' if successful
                db      ' WORLD'
debug_out       ds      100

; vim: ts=8 sw=8 sts=8 et
