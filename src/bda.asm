        if BUS == BUS_MAIN

BDA_NEUTRALDATA set     (CTH|CTL|CUP|CDOWN|CLEFT|CRIGHT)
BDA_NEUTRALCTRL set     (CTHINT|CTR)

BDA_RECVDATA    set     (CTH|CTL|CUP|CDOWN|CLEFT|CRIGHT)
BDA_RECVCTRL    set     (CTL|CTR)

BDA_SENDDATA    set     (CTH|CTL|CTR|CUP|CDOWN|CLEFT|CRIGHT)
BDA_SENDCTRL    set     (CTL|CTR|CUP|CDOWN|CLEFT|CRIGHT)


; Call this to initialize the monitor and setup gamepad link
; Level 2 interrupts must be enabled within VDP
bda_init                                        ; Call at console initialization
        if TARGET != TARGET_GEN
        if TARGET == TARGET_SCD1 || target == TARGET_SCD2
                ; Disable write protection and access bank 0
                move.b  #$00, GA_MM

                ; Get sub CPU BUSREQ
                move.w  #GA_BUSREQ|GA_NORESET, GA_RH
.waitbus
                move.b  GA_RH + 1, d0
                cmpi.b  #GA_BUSREQ|GA_NORESET, d0
                bne.b   .waitbus

                ; Copy sub CPU monitor to sub CPU RAM
                lea     bda_sub_code_source(pc), a1
                lea     bda_sub_code + $020000, a0
                move.w  #(bda_sub_code_source_end-bda_sub_code_source) / 2 - 1, d0
.3              move.w  (a1)+, (a0)+
                dbra    d0, .3

                cmpi.l  #$200, $020068
                blo.b   .damaged_l2

                ; Copy current level 2 exception vector to the new sub code handler
                move.l  $020068, bda_l2_jmp + 2 + $20000
.damaged_l2

                ; Set level 2 exception vector for sub CPU
                move.l  #bda_sub_l2, $020068

                ; Set TRACE exception vector for sub CPU
                move.l  #bda_sub_wait, $020024

                ; Set TRAP #7 exception vector for sub CPU
                move.l  #bda_sub_trap, $02009C

                ; Reset output buffer size
                clr.w   $020030

                ; Release BUSREQ
                move.w  #GA_NORESET, GA_RH

        endif   ; TARGET == TARGET_SCD1/2

                ; Initialize main CPU interrupt vectors
                lea     $24, a0
                move.l  #int_trace, d0
                jsr     bda_set_int_vector
                lea     $68, a0
                move.l  #int_pad, d0
                jsr     bda_set_int_vector
                lea     $9C, a0
                move.l  #int_trap07, d0
                jsr     bda_set_int_vector

                ; Copy monitor code from source to RAM
                lea     bda_code_source(pc), a1 ; Source
                lea     bda_code.w, a0          ; Destination at the end of RAM
                move.w  #(bda_code_source_end-bda_code_source) / 2 - 1, d0
.1              move.w  (a1)+, (a0)+
                dbra    d0, .1
        endif   ; TARGET != TARGET_GEN

                ; Setup gamepad port in neutral state (TR low, all other pins input, enable interrupt)
                move.b  #BDA_NEUTRALDATA, BDADATA               ; Set all high except TR low
                move.b  #BDA_NEUTRALCTRL, BDACTRL       ; Enable level 2 interrupts and set TL+TR as output

                rts

        if TARGET != TARGET_GEN
bda_set_int_vector
                cmp.l   (a0), d0
                beq.b   .vector_already_set
                move.l  (a0), a0                ; Point at exception handler
                move.w  #$4EF9, (a0)+           ; Write JMP instruction
                move.l  d0, (a0)                ; Write JMP target
.vector_already_set
                rts

; From now on, this code will be run from RAM, copied by the init routine
bda_code_source
                rorg    $FFFDBE
        endif   ; TARGET != TARGET_GEN
bda_code

                ; Register usage during monitor operation

                ;  d0 GP / byte to send
                ;  d1 GP
                ;  d2 GP
                ;  d3 burst length
                ;  d4 $00000006 (bit number of the TH line)
                ;  d5 $00000007 (long transfert size mask)
                ;  d6 $0000001F (byte transfert size mask)
                ;  d7 $0000000F (word trasfert size mask)

                ;  a0 general purpose
                ;  a1 general purpose
                ;  a2 bda_recvbyte
                ;  a3 bda_sendhead
                ;  a4 call return address register
                ;  a5 data port
                ;  a6 control port
                ;  a7 bda packet header

sendhead        macro
                lea     .\?(pc), a4
                jmp     (a3)
.\?
                endm

sendhead_then   macro   tailcall
                lea     tailcall(pc), a4
                jmp     (a3)
.\?
                endm

recvbyte        macro
                lea     .\?(pc), a4
                jmp     (a2)
.\?
                endm

bracall         macro   dest
                lea     .\?(pc), a4
                bra.b   dest
.\?
                endm

tailcall        macro   dest, tail
                lea     tail(pc), a4
                bra.b   dest
                endm

subret          macro
                jmp     (a4)
                endm

waitack         macro
.\?             btst    d4, (a5)
                beq.b   .\?
                endm


bda_recvbyte
.1              btst    d4, (a5)                ; Wait until clock low
                bne.b   .1
                move.b  (a5), d0                ; Read high nybble
                andi.b  #~CTL, (a5)             ; Pulse ack to low
.2              btst    d4, (a5)                ; Wait until clock high
                beq.b   .2
                move.b  (a5), d1                ; Read low nybble
                ori.b   #CTL, (a5)              ; Pulse ack to high
                and.b   d7, d1                  ; Merge nybbles (D0+D1 => D0)
                lsl.b   #4, d0
                or.b    d1, d0
                subret

; Execute a write command : parse header and write data to RAM
; d3.b = first byte of the header
bda_write
                movea.l (a7), a0                ; a0 point to destination address
                btst    d5, d3
                beq.b   bda_writebytes          ; 0xx => bytes
                btst    d4, d3
                beq.b   bda_writelongs          ; x0x => longs

bda_writewords
                lsr.b   #1, d3
                subq.b  #1, d3
                and.w   d7, d3                  ; d3 = word counter
.1              moveq   #1, d2                  ; d2 = byte counter
.2              lsl.w   #8, d0
                recvbyte
                dbra    d2, .2
                move.w  d0, (a0)+               ; Do the write
                dbra    d3, .1                  ; Write next long
                bra.w   bda_readcmd

bda_writelongs
                lsr.b   #2, d3
                subq.b  #1, d3
                and.w   d5, d3                  ; d3 = long counter
.1              moveq   #3, d2                  ; d2 = byte counter
.2              lsl.l   #8, d0
                recvbyte
                dbra    d2, .2
                move.l  d0, (a0)+               ; Do the write
                dbra    d3, .1                  ; Write next long
                bra.b   bda_readcmd

bda_writebytes
                subq.b  #1, d3
                and.w   d6, d3                  ; d3 = byte counter
.1              recvbyte
                move.b  d0, (a0)+               ; Do the write
                dbra    d3, .1                  ; Write next byte
                bra.b   bda_readcmd


; Gamepad interrupt
int_pad
                clr.l   bda_pkt_header.w        ; No command callback
                bra.b   bda_enter
; Breakpoint interrupt
int_trace
                move.l  #$09, bda_pkt_header.w
                bra.b   bda_enter
int_trap07
                move.l  #$27, bda_pkt_header.w  ; Send TRAP 7 code once entered monitor
bda_enter
                move.w  #$2700, sr              ; Disable all interrupts

                move.w  (a7)+, bda_sr.w         ; Copy SR value to a fixed address
                move.l  (a7)+, bda_pc.w         ; Copy PC to a fixed address
                move.l  a7, bda_a7.w            ; Store application stack value
                lea     bda_a7.w, a7            ; Use monitor stack
                movem.l d0-d7/a0-a6, -(a7)      ; Push registers to bda_regs
                subq.l  #4, a7                  ; Point a7 to bda_pkt_header
                ; Load frequently used constants in registers (to save code space)
                moveq.l #$00000006, d4          ; Bit number of TH line (CTH_BIT)
                moveq.l #$00000007, d5          ; Longs per packet
                moveq.l #$0000001F, d6          ; Bytes per packet
                moveq.l #$0000000F, d7          ; Words per packet
                lea     bda_recvbyte(pc), a2
                lea     bda_sendhead(pc), a3
                lea     BDADATA, a5
                lea     BDACTRL, a6

                tst.l   (a7)                    ; Test if a message was pre-buffered
                beq.b   bda_readcmd
                sendhead                        ; Send the pre-buffered message

bda_finishwrite

                waitack

bda_readcmd
                ; Setup port in input mode
                move.b  #BDA_RECVDATA, (a5)     ; Release all pins high and TR low
                move.b  #BDA_RECVCTRL, (a6)     ; Set ack to output

                ; Wait the sender to pull TH
                ; or sub CPU to enter monitor
        if TARGET == TARGET_SCD1 || TARGET == TARGET_SCD2
                lea     GA_COMMFLAGS + 1, a0
        endif
.waitevt          
        if TARGET == TARGET_SCD1 || TARGET == TARGET_SCD2
                btst    #6, (a0)
                beq.b   .nosub
                ori.b   #$C0, -1(a0)            ; Acknowledge
.waitack        btst    #6, (a0)                ; Wait until flag is reset
                bne.b   .waitack
                bclr    #6, -1(a0)              ; Clear ack flag
                move.l  #$00000127, (a7)
                sendhead_then bda_finishwrite
.nosub
        endif
                btst    d4, (a5)
                bne.b   .waitevt

                ; Read the incoming command + address (4 bytes)
                moveq   #3, d2
                move.l  a7, a0                  ; a0 points to data to be sent
.2              recvbyte
                move.b  d0, (a0)+
                dbra    d2, .2

                move.b  (a7), d3                ; Read the first byte of the header
                move.b  d3, d0                  ; Test command
                andi.b  #$C0, d0
                beq.b   bda_escape              ; 00xxxxxx = escape mode
                btst    #5, d3                  ; xx1xxxxx = write, xx0xxxxx = read
                bne.w   bda_write
                bra.b   bda_send                ; Send the requested data

bda_escape
                btst    #5, d3                  ; 001xxxxx = exit, 000xxxxx = handshake
                bne.b   bda_exit

                sendhead_then bda_finishwrite   ; Pong : reply with the same command

; Exit from monitor
bda_exit
                move.l  #$20000000, (a7)
                sendhead                        ; Signal that we leave monitor mode

                ; Wait until the exit command has been acknowledged
                waitack

                ; Set pad pins back to neutral state
                move.b  #BDA_NEUTRALDATA, (a5)
                move.b  #BDA_NEUTRALCTRL, (a6)

                addq.l  #4, a7                  ; Point a7 back to registers
                movem.l (a7)+, d0-d7/a0-a6      ; Pop registers
                move.l  bda_a7.w, a7            ; Restore stack pointer
                move.l  bda_pc.w, -(a7)         ; Restore PC
                move.w  bda_sr.w, -(a7)         ; Restore SR

                rte                             ; Return from interrupt
; End of monitor main loop


; Parse header and send data
bda_send
                bset    #5, (a7)                ; Set the write flag right into header
                sendhead                        ; Send updated header with the write command

                move.l  (a7), a1                ; Read source address from header
                move.b  (a7), d3                ; Read first header byte

                ; Read word size
                btst    d5, d3
                beq.b   bda_sendbytes
                btst    d4, d3
                beq.b   bda_sendlongs

; Send words
bda_sendwords   lsr.b   #1, d3                  ; Convert d3 from byte count to word count
                subq    #1, d3                  ; Adjust for DBRA
                and.w   d7, d3                  ; Filter out bits
.1              move.w  (a1)+, d0               ; Read value with a word operation
                move.w  d0, (a7)                ; Move data to header buffer
                moveq   #1, d2                  ; Write 2 bytes
                move.l  a7, a0                  ; a0 points to data to be sent
                bracall bda_sendbuf             ; Send value in header
                dbra    d3, .1
                bra.w   bda_finishwrite

; Send byte by byte
bda_sendbytes   subq    #1, d3                  ; Adjust for DBRA
                and.w   d6, d3                  ; Filter out bits
                move.w  d3, d2
                movea.l a1, a0
                tailcall        bda_sendbuf, bda_finishwrite            ; Send buffer byte per byte

; Send longs
bda_sendlongs   lsr.b   #2, d3                  ; Convert d3 from byte count to long count
                subq    #1, d3                  ; Adjust for DBRA
                and.w   d5, d3                  ; Filter out bits
.1              move.l  (a1)+, (a7)             ; Read value to header
                moveq   #3, d2                  ; Write 4 bytes
                move.l  a7, a0                  ; a0 points to data to be sent
                bracall bda_sendbuf             ; Send value in header
                dbra    d3, .1
                bra.w   bda_finishwrite

; Send the 4 byte header
bda_sendhead
                ; Setup port in output mode
                move.b  #BDA_SENDDATA, (a5)     ; Raise TR
                move.b  #BDA_SENDCTRL, (a6)     ; Set all pins except TH as output

                moveq   #3, d2                  ; Write 4 bytes
                move.l  a7, a0                  ; a0 points to data to be sent

; Send memory buffer (a0 = address, d2 = byte count)
bda_sendbuf
                move.b  (a0)+, d0
                move.b  d0, d1                  ; d1 is used as scratch register
                lsr.b   #4, d1                  ; Prepare high nybble
                ori.b   #CTL|CTR, d1            ;

                waitack

.1              move.b  d1, (a5)                ; Place value
                eori.b  #CTL, d1
                move.b  d1, (a5)                ; Pulse TL to low (clock)

                ; Prepare low nybble
                and.b   d7, d0
                ori.b   #CTR, d0

.2              cmp.b   (a5), d1                ; Wait for ack (wait until TH = 0)
                bne.b   .2

                move.b  d0, (a5)                ; Set low nybble (ack will be handled on next write cycle)
                eori.b  #CTL, d0
                move.b  d0, (a5)                ; Pulse TL to high (clock)

                dbra    d2, bda_sendbuf
                subret

bda_code_end
        assert bda_code_end <= $FFFFB6
        if TARGET != TARGET_GEN
                rend
bda_code_source_end
        endif


                ; Sub CPU code

        if TARGET == TARGET_SCD1 || TARGET == TARGET_SCD2
BUS     set BUS_SUB
bdasub_d0       set     $0000C0                 ; Place monitor after exception vectors

bdasub_sp       set     bdasub_d0 + 15*4
bdasub_pc       set     bdasub_d0 + 16*4
bdasub_sr       set     bdasub_d0 + 17*4

BDA_COMM_MAIN   set     $FFFF800E
BDA_COMM_SUB    set     $FFFF800F

                ; Mark the beginning of bda sub code
                ; Used by update_bda_sub in bdp.c
                hex     0000 1234 DEAD BEEF
                asciiz  'BDA_SUB'

bda_sub_code_source
                rorg    bdasub_sr + 2           ; Place code after CPU state
bda_sub_code

bda_sub_reset   ; Reset comm flags
                ; Called only by BDB
                lea     BDA_COMM_SUB, a0
                clr.b   (a0)
                lea     $11(a0), a0
                move.w  #3, d0
.clear_loop     clr.l   (a0)+
                dbra    d0, .clear_loop

bda_sub_wait    move.w  #$2700, sr              ; Disable all interrupts
                move.w  (a7)+, bdasub_sr.w      ; Copy SR value to a fixed address
                move.l  (a7)+, bdasub_pc.w      ; Copy PC to a fixed address
                movem.l d0-d7/a0-a7, bdasub_d0.w; Push registers to bda_regs

                ; Enable pause flag
                bset    #7, BDA_COMM_SUB

                ; Wait until main sets the flag
                ; It can be clear in case of voluntary exception
.0              btst    #7, BDA_COMM_MAIN
                beq.b   .0

                ; Pause in monitor mode

.2              btst    #6, BDA_COMM_MAIN       ; Manage ack
                beq.b   .1
                bclr    #6, BDA_COMM_SUB        ; Main CPU acknowledged trap: clear the flag
.1
                addi.w  #1, $1FE.w
                btst    #7, BDA_COMM_MAIN       ; Wait until main CPU releases pause
                bne.b   .2

                ; Execution resumed

                ; Clear pause and trap flags
                andi.b  #$3F, BDA_COMM_SUB

                movem.l bdasub_d0.w, d0-d7/a0-a7; Pop registers
                move.l  bdasub_pc.w, -(a7)      ; Restore PC
                move.w  bdasub_sr.w, -(a7)      ; Restore SR

                rte

bda_sub_l2
                btst    #7, BDA_COMM_MAIN
                bne.b   bda_sub_wait            ; If the bit is set, go to monitor mode

bda_l2_jmp      jmp     $5F7C.l                 ; The JMP target will be patched by bda_init to jump at the official L2 handler


bda_sub_trap    ; Voluntary interruption
                bset    #6, BDA_COMM_SUB
                jmp     bda_sub_wait

bda_sub_code_end
                assert  bda_sub_code_end <= SUB_SCRATCH ; Ensure subcode fits in the header, before scratch pad
                rend
bda_sub_code_source_end
BUS     set BUS_MAIN

                ; Entry point offsets for update_bda_sub
                dl      bda_sub_reset           ; Reset vector offset
                dl      bda_sub_wait            ; Trace vector offset
                dl      bda_sub_l2              ; L2 vector offset
                dl      bda_sub_trap            ; TRAP #7 vector offset
                ; Marker for the end of bda sub code
                hex     CAFE BABE

        endif   ; TARGET == TARGET_SCD1/2

        endif   ; BUS == BUS_MAIN

; vim: ts=8 sw=8 sts=8 et
