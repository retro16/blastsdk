BDP_OUT_BUFSIZE equ     $30     ; Address of data size in buffer
BDP_OUT_BUFFER  equ     $32     ; Address of sub buffer
BDP_OUT_MAXLEN  equ     $2E     ; Max number of bytes in buffer

; void bdp_write(char *data, int length);
_sub_bdp_write
                ; Use the reserved exception vector area as a buffer

                move.l  8(sp), d1               ; d1 = length
                bne.b   .data_present
                rts
.data_present
                move.l  4(sp), a0               ; a0 = source data

                move    sr, d0
                move.w  d0, -(sp)               ; Save SR
                ori     #$0700, sr              ; Disable interrupts

.next_block
                move.w  #BDP_OUT_MAXLEN, d0
                cmp.w   d0, d1
                bhi.b   .big_data               ; Buffer smaller than data
                move.w  d1, d0                  ; Copy the whole remaining data
                beq.b   .finished               ; Quit if no more data to copy
.big_data       sub.w   d0, d1                  ; Compute number of bytes
                                                ; remaining after copy

.copy_start     lea     BDP_OUT_BUFSIZE, a1
                move.w  d0, (a1)+               ; Tell main CPU current block size
                subq.w  #1, d0                  ; Adjust d0 for dbra
.copy_loop      move.b  (a0)+, (a1)+            ; Copy data to buffer
                dbra    d0, .copy_loop

                bset    #5, GA_COMMFLAGS_SUB    ; Tell main CPU that data is ready
.wait_flush     tst.w   BDP_OUT_BUFSIZE
                bne.b   .wait_flush             ; Wait until main cpu sent data
                bclr    #5, GA_COMMFLAGS_SUB

                bra.b   .next_block
.finished
                move.w  (sp)+, d0               ; Restore sr
                move    d0, sr
                rts

; vim: ts=8 sw=8 sts=8 et
