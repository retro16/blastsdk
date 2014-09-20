		if BUS == BUS_MAIN

; Set port used for communication
BDPDATA	set	CDATA2
BDPCTRL	set	CCTRL2

BDP_NEUTRALDATA	set	(CTH|CTL|CUP|CDOWN|CLEFT|CRIGHT)
BDP_NEUTRALCTRL	set	(CTHINT|CTR)

BDP_SENDDATA    set (CTH|CTL|CTR|CUP|CDOWN|CLEFT|CRIGHT)
BDP_SENDCTRL    set (CTL|CTR|CUP|CDOWN|CLEFT|CRIGHT)

; void bdp_write(char *data, int length);
bdp_write
		move.l	8(sp), d1	; d1 = length
		bne.b .data_present
		rts
.data_present
		move.l	4(sp), a0	; a0 = source data

		; Mask interrupts
		move	sr, d0
		move	#$2700, sr
		move.w	d0, -(a7)

		; Setup port in output mode
		move.b	#BDP_SENDDATA, BDPDATA
		move.b	#BDP_SENDCTRL, BDPCTRL

		; Compute packet size
bdp_send_packet
		move.w	d1, d0	; d0 = packet size
		cmpi.l	#3, d1
		ble.b		.small
		moveq		#3, d0	; Max 3 bytes per packet
.small

bdp_send_raw_packet	; Entry point to send one raw packet (d0 = low nybble of first byte, d1 = 3, a0 points to data)

		; Send header byte
		move.b	#0|CTR|CTH, BDPDATA	; Send high nybble (always 0)
		ori.b	#CTL|CTR|CTH, d0	; Precompute next nybble (packet size)

.head_msb
		btst	#CTH_BIT, BDPDATA	; Wait ack for high nybble
		bne.b	.head_msb
		move.b	d0, BDPDATA

		moveq	#2, d0	; Always send 3 bytes per packet
bdp_send_byte
		swap	d0	; Use other half of d0

		; Wait until last byte was acknowledged
.wait_last_ack
		btst	#CTH_BIT, BDPDATA
		beq.b	.wait_last_ack

		; Send high nybble
		move.b	(a0), d0
		lsr.b	#4, d0
		ori.b	#CTR|CTH, d0
		move.b	d0, BDPDATA	; Send high nybble of data

		; Precompute low nybble
		move.b	(a0)+, d0
		andi.b	#$0F, d0
		ori.b	#CTL|CTR|CTH, d0

		; Wait for high nybble ack
.byte_msb
		btst	#CTH_BIT, BDPDATA
		bne.b	.byte_msb

		move.b	d0, BDPDATA	; Send low nybble of data

		swap	d0	; Restore byte count in d0
		dbra	d0, bdp_send_byte

		; Packet sent
		subq.w	#3, d1	; Compute remaining bytes to send
		bgt.w	bdp_send_packet

bdp_write_finished
		; Put port back to neutral state
		move.b	#BDP_NEUTRALDATA, BDPDATA
		move.b	#BDP_NEUTRALCTRL, BDPCTRL

		; Unmask interrupts
		move.w	(a7)+, d0
		move	d0, sr

		rts


; Poll sub cpu state
; void bdp_sub_check();
bdp_sub_check
		andi.b	#$E0, GA_COMMFLAGS_SUB	; Quickly check sub CPU flags
		bne.w	.int
		rts

.int
		; mask interrupts
		move	sr, d0
		move	#$2700, sr
		move.w	d0, -(a7)

		; setup port in output mode
		move.b	#BDP_SENDDATA, BDPDATA
		move.b	#BDP_SENDCTRL, BDPCTRL

		; prepare raw packet
		moveq	#3, d1
		moveq	#0, d0

		; Point a0 at data to be sent and call bdp_send_raw_packet
		btst	#6, GA_COMMFLAGS_SUB
		beq.b	.notrace
		; sub cpu raised a trace exception
		lea	.sub_trace + 1, a0
		pea	.bda_check_end
		jmp	bdp_send_raw_packet
.notrace

		; sub cpu entered monitor by itself (trap #7)
		lea	.sub_trap7 + 1, a0
		pea	.bda_check_end
		jmp	bdp_send_raw_packet

.sub_trace
		dl	$109
.sub_trap7
		dl	$127
.bda_check_end

		btst	#5, GA_COMMFLAGS_SUB	; test if sub buffer contains data
		beq.b	.noout

		; Request program ram bank 0
		SUB_ACCESS_PRAM 0

		; Send data to the debugger
		pea	#$020032	; Push data address
		moveq	#0, d0	; Push data size
		move.w	$020030, d0
		move.l	d0, -(a7)
		jsr	bdp_write

		; Acknowledge to the sub cpu by resetting data size
		clr.w	#$020030

		; Release sub cpu
		SUB_RELEASE_PRAM

.noout
		rts

		elsif BUS == BUS_SUB

; void bdp_write(char *data, int length);
bdp_write

		; Use the reserved exception vector area as a buffer
BDP_OUT_BUFSIZE	equ	$30
BDP_OUT_BUFFER	equ	$32
BDP_OUT_MAXSIZE	equ	$2E

		move.l	8(sp), d1	; d1 = length
		bne.b .data_present
		rts
.data_present
		move.l	4(sp), a0	; a0 = source data


		endif
