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
		
		; Send header byte
		move.b	#0|CTR|CTH, BDPDATA	; Send high nybble (always 0)
		ori.b	#CTL|CTR|CTH, d0	; Precompute next nybble (packet size)
		
		VDPSETBG	$080
.head_msb
		btst	#BCTH, BDPDATA	; Wait ack for high nybble
		bne.b	.head_msb
		move.b	d0, BDPDATA
		
		moveq	#2, d0	; Always send 3 bytes per packet
bdp_send_byte
		swap	d0	; Use other half of d0
		VDPSETBG	$008

		; Wait until last byte was acknowledged
.wait_last_ack
		btst	#BCTH, BDPDATA
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

		VDPSETBG	$800
		
		; Wait for high nybble ack
.byte_msb
		btst	#BCTH, BDPDATA
		bne.b	.byte_msb
		VDPSETBG	$808

		move.b	d0, BDPDATA	; Send low nybble of data
		
		swap	d0	; Restore byte count in d0
		dbra	d0, bdp_send_byte

		VDPSETBG	$088

		; Packet sent
		subq.w	#3, d1	; Compute remaining bytes to send
		bgt.w	bdp_send_packet
		VDPSETBG	$880
		
bdp_write_finished
		; Put port back to neutral state
		move.b	#BDP_NEUTRALDATA, BDPDATA
		move.b	#BDP_NEUTRALCTRL, BDPCTRL
		
		; Unmask interrupts
		move.w	(a7)+, d0
		move	d0, sr
		VDPSETBG	$888
		
		rts
