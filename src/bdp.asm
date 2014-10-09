BDP_OUT_BUFSIZE equ     $30     ; Address of data size in buffer
BDP_OUT_BUFFER  equ     $32     ; Address of sub buffer
BDP_OUT_MAXSIZE equ     $2E     ; Max number of bytes in buffer

; Set port used for communication
BDPDATA set     CDATA2
BDPCTRL set     CCTRL2

BDP_NEUTRALDATA set     (CTH|CTL|CUP|CDOWN|CLEFT|CRIGHT)
BDP_NEUTRALCTRL set     (CTHINT|CTR)

BDP_SENDDATA    set (CTH|CTL|CTR|CUP|CDOWN|CLEFT|CRIGHT)
BDP_SENDCTRL    set (CTL|CTR|CUP|CDOWN|CLEFT|CRIGHT)

; void bdp_write(char *data, int length);
_main_bdp_write
                move.l  8(sp), d1       ; d1 = length
                bne.b .data_present
                rts
.data_present
                move.l  4(sp), a0       ; a0 = source data

                ; Mask interrupts
                move    sr, d0
                move.w  d0, -(sp)
                move    #$2700, sr

                ; Setup port in output mode
                move.b  #BDP_SENDDATA, BDPDATA
                move.b  #BDP_SENDCTRL, BDPCTRL

                ; Compute packet size
bdp_send_packet
                move.w  d1, d0                  ; d0 = packet size
                cmpi.l  #3, d1
                ble.b   .small
                moveq   #3, d0                  ; Max 3 bytes per packet
.small

bdp_send_raw_packet     ; Entry point to send one raw packet (d0 = low nybble of first byte, d1 = 3, a0 points to data)

                ; Send header byte
                move.b  #0|CTR|CTH, BDPDATA     ; Send high nybble (always 0)
                ori.b   #CTL|CTR|CTH, d0        ; Precompute next nybble (packet size)

.head_msb
                btst    #CTH_BIT, BDPDATA       ; Wait ack for high nybble
                bne.b   .head_msb
                move.b  d0, BDPDATA

                moveq   #2, d0                  ; Always send 3 bytes per packet
bdp_send_byte
                swap    d0                      ; Use other half of d0

                ; Wait until last byte was acknowledged
.wait_last_ack
                btst    #CTH_BIT, BDPDATA
                beq.b   .wait_last_ack

                ; Send high nybble
                move.b  (a0), d0
                lsr.b   #4, d0
                ori.b   #CTR|CTH, d0
                move.b  d0, BDPDATA             ; Send high nybble of data

                ; Precompute low nybble
                move.b  (a0)+, d0
                andi.b  #$0F, d0
                ori.b   #CTL|CTR|CTH, d0

                ; Wait for high nybble ack
.byte_msb
                btst    #CTH_BIT, BDPDATA
                bne.b   .byte_msb

                move.b  d0, BDPDATA             ; Send low nybble of data

                swap    d0                      ; Restore byte count in d0
                dbra    d0, bdp_send_byte

                ; Packet sent
                subq.w  #3, d1                  ; Compute remaining bytes to send
                bgt.b   bdp_send_packet

bdp_write_finished
                ; Put port back to neutral state
                move.b  #BDP_NEUTRALDATA, BDPDATA
                move.b  #BDP_NEUTRALCTRL, BDPCTRL

                ; Unmask interrupts
                move.w  (sp)+, d0
                move    d0, sr

                rts


        if TARGET == TARGET_SCD1 || TARGET == TARGET_SCD2

; Poll sub cpu state
; void bdp_sub_check();
bdp_sub_check
                btst    #6, GA_COMMFLAGS_SUB
                beq.b   .bda_check_end

                ori.b   #$C0, GA_COMMFLAGS_MAIN ; Ack the event
.wait_trap_ack  btst    #6, GA_COMMFLAGS_SUB
                bne.b   .wait_trap_ack
                bclr    #6, GA_COMMFLAGS_MAIN   ; Clear ack

                ; mask interrupts
                move    sr, d0
                move    #$2700, sr

                ; setup port in output mode
                move.b  #BDP_SENDDATA, BDPDATA
                move.b  #BDP_SENDCTRL, BDPCTRL

                ; prepare raw packet parameters
                moveq   #3, d1
                lea     .sub_trap7 + 1, a0
                pea     .bda_check_end          ; Tail call and return to .bda_check_end
                move.w  d0, -(sp)               ; bdp_send_packet will restore SR from the stack
                moveq   #0, d0                  ; d0 contains the header byte
                jmp     bdp_send_raw_packet
.sub_trap7
                dl      $127
.bda_check_end

                btst    #5, GA_COMMFLAGS_SUB    ; test if sub buffer contains data
                beq.b   .noout

                ; Request program ram bank 0
                SUB_ACCESS_PRAM 0

                tst.w   $020000 + BDP_OUT_BUFSIZE
                beq.b   .emptybuf

                ; Send data to the debugger
                moveq   #0, d0
                move.w  $020000 + BDP_OUT_BUFSIZE, d0   ; Read data size
                move.l  d0, -(sp)                       ; Push data size
                pea.l   $020000 + BDP_OUT_BUFFER        ; Push data buffer address
                jsr     _main_bdp_write                 ; Call function
                addq    #8, sp

                ; Acknowledge to the sub cpu by resetting data size
                clr.w   $020000 + BDP_OUT_BUFSIZE

                ; Release sub cpu
.emptybuf       SUB_RELEASE_PRAM

.noout
                rts
        endif

; vim: ts=8 sw=8 sts=8 et
