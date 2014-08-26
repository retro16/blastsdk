		; Debug protocol control and data ports
		; To use the "serial" port of the old genesis, specify CCTRL3 / CDATA3

	if BUS == BUS_MAIN

BDPCTRL		equ	CCTRL2
BDPDATA		equ	CDATA2

; Call this to initialize the monitor and setup gamepad link
; Level 2 interrupts must be enabled within VDP
bda_init					; Call at console initialization
	if SCD != 0
		; Copy monitor code from source to RAM
		lea	bda_code_source(pc), a1	; Source
		lea	bda_code.w, a0		; Destination at the end of RAM
		move.w	#(bda_code_source_end-bda_code_source) / 2 - 1, d0
.1		move.w	(a1)+, (a0)+
		dbra	d0, .1

		; Disable write protection and access bank 0
		andi.w	#$003F, GA_MM

		; Get sub CPU BUSREQ
		move.w	#GA_BUSREQ|GA_NORESET, GA_RH

		; Initialize Sega CD interrupt vectors
		move.l	#g_int_pad, G_INTV_PAD
		move.l	#g_int_trace, G_INTV_TRACE
		move.l	#g_int_trap07, G_INTV_TRAP07

		; Reset comm flags
		move.w	#0, GA_COMMFLAGS

		; Copy sub CPU monitor to sub CPU RAM
		lea	bda_sub_code_source(pc), a1
		lea	bda_sub_code + $020000, a0
		move.w	#(bda_sub_code_source_end-bda_sub_code_source) / 2 - 1, d0
.3		move.w	(a1)+, (a0)+
		dbra	d0, .3

		; Set level 2 exception vector for sub CPU
		move.l	#bda_sub_l2, $020068

		; Set TRAP #7 exception vector for sub CPU
		move.l	#bda_sub_trap, $02009C

		; Release BUSREQ
		move.w	#GA_NORESET, GA_RH

	endif	; SCD != 0

		; Setup gamepad port
		move.b	#$7F, BDPDATA		; Reset all outputs to high
		move.b	#(CTHINT), BDPCTRL	; Enable level 2 interrupts

		rts


	if SCD != 0
; From now on, this code will be run from RAM, copied by the init routine
bda_code_source
		rorg	$FFFDBE
	endif	; SCD != 0
bda_code

		; Register usage during monitor operation

		;  d0 GP / byte to send
		;  d1
		;  d2
		;  d3
		;  d4 burst length
		;  d5 $00000007 (long transfert size mask)
		;  d6 $0000001F (byte transfert size mask)
		;  d7 $0000000F (word trasfert size mask)

		;  a0 general purpose
		;  a1 general purpose
		;  a2 bda_readbyte
		;  a3 bda_sendhead
		;  a4 call return address register
		;  a5 data port
		;  a6 control port
		;  a7 bda packet header

sendhead	macro
		lea	.\?(pc), a4
		jmp	(a3)
.\?
		endm

readbyte	macro
		lea	.\?(pc), a4
		jmp	(a2)
.\?
		endm

bracall		macro	dest
		lea	.\?(pc), a4
		bra.b	dest
.\?
		endm

subret		macro
		jmp	(a4)
		endm


bda_readbyte
.1		btst	#BCTH, (a5)
		bne.b	.1
		move.b	(a5), d0
		eori.b	#CTL, (a5)		; Pulse ack
		lsl.b	#4, d0
.2		btst	#BCTH, (a5)
		beq.b	.2
		move.b	(a5), d1
		eori.b	#CTL, (a5)		; Pulse ack
		and.b	d7, d1			; Merge nybbles
		or.b	d1, d0
		subret

; Execute a write command : parse header and write data to RAM
; d4.b = first byte of the header
bda_write
		movea.l	(a7), a0		; a0 point to destination address
		btst	#7, d4
		beq.b	bda_writebytes		; 0xx => bytes
		btst	#6, d4
		beq.b	bda_writelongs		; x0x => longs

bda_writewords
		lsr.b	#1, d4
		subq.b	#1, d4
		and.w	d7, d4			; d4 = word counter
.1		moveq	#1, d3			; d3 = byte counter
.2		lsl.l	#8, d2
		readbyte
		move.b	d0, d2
		dbra	d3, .2
		move.w	d2, (a0)+		; Do the write
		dbra	d4, .1			; Write next long
		bra.w	bda_wait

bda_writelongs
		lsr.b	#2, d4
		subq.b	#1, d4
		and.w	d5, d4			; d4 = long counter
.1		moveq	#3, d3			; d3 = byte counter
.2		lsl.l	#8, d2
		readbyte
		move.b	d0, d2
		dbra	d3, .2
		move.l	d2, (a0)+		; Do the write
		dbra	d4, .1			; Write next long
		bra.b	bda_wait

bda_writebytes
		subq.b	#1, d4
		and.w	d6, d4			; d4 = byte counter
.1		readbyte
		move.b	d0, (a0)+		; Do the write
		dbra	d4, .1			; Write next byte
		bra.b	bda_wait


; Gamepad interrupt
g_int_pad
		clr.l	bda_pkt_header.w	; No command callback
		btst	#BCTR, BDPDATA		; Check that TR is low, indicating
						; incoming data
		beq.b	bda_enter
		rte				; Spurious interrupt : ignore

; Breakpoint interrupt
g_int_trace
		move.l	#$09, bda_pkt_header.w
		bra.b	bda_enter
g_int_trap07
		move.l	#$27, bda_pkt_header.w    ; Send TRAP 7 code once entered monitor
bda_enter
		move.w	#$2700, sr		; Disable all interrupts

		move.w	(a7), bda_sr.w		; Copy SR value to a fixed address
		move.l	2(a7), bda_pc.w		; Copy PC to a fixed address
		addq.l	#6, a7			; Pop interrupt stack frame
		move.l	a7, bda_sp.w		; Store application stack value
		lea	bda_sp.w, a7		; Use monitor stack
		movem.l	d0-d7/a0-a6, -(a7)	; Push registers to bda_regs
		subq.l	#4, a7			; Point a7 to bda_pkt_header
		moveq.l	#$00000007, d5
		moveq.l	#$0000001F, d6
		moveq.l	#$0000000F, d7
		lea	bda_readbyte(pc), a2
		lea	bda_sendhead(pc), a3
		lea	BDPDATA, a5
		lea	BDPCTRL, a6

		tst.l	(a7)			; Test if a message was pre-buffered
		beq.b	bda_wait
		sendhead			; Send the pre-buffered message

; Wait for a command and execute it (main loop)
bda_wait
		; Setup port in input mode
		move.b	#$FF, (a5)		; Release all pins high
		move.b	#(CTL), (a6)		; Set ack to output
		; Wait the sender to pull TR
.1		btst	#BCTR, (a5)
		bne.b	.1

		; Read the incoming command
		moveq	#3, d3
		move.l	a7, a0			; a0 points to data to be sent
bda_readbuf	readbyte
		move.b	d0, (a0)+
		dbra	d3, bda_readbuf

		move.b	(a7), d4		; Read the first byte of the header
		move.b	d4, d0			; Test command
		andi.b	#$C0, d0
		beq.b	bda_escape		; 00xxxxxx = escape mode
		btst	#5, d4			; xx1xxxxx = write, xx0xxxxx = read
		bne.w	bda_write
		bra.b	bda_send		; Send the requested data

bda_escape
		btst	#5, d4			; 001xxxxx = exit, 000xxxxx = handshake
		bne.b	bda_exit

		sendhead			; Pong : reply with the same command
		bra.b	bda_wait

; Exit from monitor
bda_exit
		move.l	#$20000000, (a7)
		sendhead			; Signal that we leave monitor mode

bda_abort					; Jump here directly for panic exit

		move.b	#(CTHINT), BDPCTRL	; Reset to interrupt mode, all pins as input
		lea	bda_d0.w, a7		; Restore a7 in case of corruption
		movem.l	(a7)+, d0-d7/a0-a6	; Pop registers
                move.l  bda_sp.w, a7
		subq.l	#6, a7			; Restore interrupt stack frame
		move.w	bda_sr.w, (a7)		; Restore SR in case it was changed by the monitor
		move.l	bda_pc.w, 2(a7)		; Restore PC

		rte				; Return from interrupt
; End of monitor main loop


; Monitor subroutines

; Reads a byte and returns it in d0
; Alters d1
; Tested : OK


; Parse header and send data
bda_send
		bset	#5, (a7)		; Set the write flag right into header
		sendhead			; Send updated header with the write command

		move.l	(a7), a1		; Read source address from header
		move.b	(a7), d4		; Read first header byte

		; Read word size
		btst	#7, d4
		beq.b	bda_sendbytes
		btst	#6, d4
		beq.b	bda_sendlongs

; Send words
bda_sendwords	lsr.b	#1, d4			; Convert d4 from byte count to word count
		subq	#1, d4			; Adjust for DBRA
		and.w	d7, d4			; Filter out bits
.1		move.w	(a1)+, d0		; Read value with a word operation
		move.w	d0, (a7)		; Move data to header buffer
		moveq	#1, d3			; Write 2 bytes
		move.l	a7, a0			; a0 points to data to be sent
		bracall	bda_sendbuf		; Send value in header
		dbra	d4, .1
		bra.w	bda_wait
		
; Send byte by byte
bda_sendbytes	subq	#1, d4			; Adjust for DBRA
		and.w	d6, d4			; Filter out bits
		move.w	d4, d3
		movea.l	a1, a0
		bracall	bda_sendbuf		; Send buffer byte per byte
		bra.w	bda_wait

; Send longs
bda_sendlongs	lsr.b	#2, d4			; Convert d4 from byte count to long count
		subq	#1, d4			; Adjust for DBRA
		and.w	d5, d4			; Filter out bits
.1		move.l	(a1)+, (a7)		; Read value to header
		moveq	#3, d3			; Write 4 bytes
		move.l	a7, a0			; a0 points to data to be sent
		bracall	bda_sendbuf		; Send value in header
		dbra	d4, .1
		bra.w	bda_wait

; Send the 4 byte header
bda_sendhead
		; Wait the sender to release TR
.1		btst	#BCTR, (a5)
		beq.b	.1
		; Setup port in output mode
		move.b	#$FF, (a5)		; Release all pins high
		move.b	#(CUP|CDOWN|CLEFT|CRIGHT|CTH), (a6)

		moveq	#3, d3			; Write 4 bytes
		move.l	a7, a0			; a0 points to data to be sent
; Send memory buffer (a0 = address, d3 = byte count)
bda_sendbuf	move.b	(a0)+, d0
		move.b	d0, d1			; d1 is used as scratch register
		lsr.b	#4, d1			; Prepare high nybble
		ori.b	#CTH|CTR, d1		; Keep clock high and wait for ack low, busreq high

.1		move.b	d1, (a5)		; Place value
		eori.b	#CTH, d1
		move.b	d1, (a5)		; Pulse clock

.2		cmp.b	(a5), d1		; Wait for ack
		bne.b	.2
		btst	#BCTR, (a5)		; Keep an eye on output mode
		beq.w	bda_abort		; Stop since the debugger is talking to us
		btst	#BCTL, d1		; If ack was high, we were sending
		bne.b	.3			; the second nybble : stop now

		move.b	d0, d1			; Load the low nybble
		and.b	d7, d1
		ori.b	#CTL|CTR, d1		; Wait for a high ack, high busreq this time
		bra.b	.1
.3
		dbra	d3, bda_sendbuf
		subret

bda_code_end
	if SCD != 0
		rend
bda_code_source_end
	endif


		; Sub CPU code

	if SCD != 0
bdasub_d0	set	$0000C0			; Place monitor after exception vectors

bdasub_sp	set	bdasub_d0 + 15*4+2
bdasub_pc	set	bdasub_d0 + 16*4+2
bdasub_sr	set	bdasub_d0 + 17*4+2

bda_sub_code_source
		rorg	bdasub_d0 + 17*4+2	; Place code after CPU state
bda_sub_code
bda_sub_l2	btst	#15, GA_COMMFLAGS
		beq.b	bda_l2_jmp		; Bit not set : call user-defined interrupt
		bclr	#7, GA_COMMFLAGS

bda_sub_wait	move.w	#$2700, sr		; Disable all interrupts
		move.w	(a7), bdasub_sr.w	; Copy SR value to a fixed address
		move.l	2(a7), bdasub_pc.w	; Copy PC to a fixed address
		addq.l	#6, a7			; Pop interrupt stack frame
		move.l	a7, bdasub_sp.w		; Store application stack value
		lea	bdasub_sp.w, a7		; Use monitor stack
		movem.l	d0-d7/a0-a6, -(a7)	; Push registers to bda_regs

.1		btst	#15, GA_COMMFLAGS	; Wait until main CPU produces a falling edge
		beq.b	.1			; on bit 15 of COMMFLAGS
.2		btst	#15, GA_COMMFLAGS	; Wait until main CPU releases pause
		bne.b	.2
						; Execution resumed
		movem.l	(a7)+, d0-d7/a0-a7	; Pop registers
		subq.l	#6, a7			; Restore interrupt stack frame
		move.w	bdasub_sr.w, (a7)	; Restore SR in case it was changed by the monitor
		move.l	bdasub_pc.w, 2(a7)	; Restore PC
		btst	#7, GA_COMMFLAGS	; If voluntary interruption
		bra.b	bda_l2_default		; Ignore exception vector call

bda_l2_jmp	jmp	bda_l2_default.l
bda_l2_default	rte
bda_sub_trap	bset	#7, GA_COMMFLAGS	; Indicate a voluntary interruption
		bra.b	bda_sub_wait
		
bda_sub_code_end
		assert	bda_sub_code_end < $200	; Ensure subcode fits in header
		rend
bda_sub_code_source_end
	endif	; SCD != 0

		; Sub CPU defines
	else	; BUS == BUS_MAIN
G_INTV_LEVEL2	set	bda_l2_jmp + 2		; Redirect exception vector to the jump
	endif	; BUS == BUS_MAIN

