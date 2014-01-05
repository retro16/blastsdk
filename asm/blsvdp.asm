;;;;;
; blsvdp_dma(void *dest, void *src, unsigned int len);
; Send data to VRAM using DMA
blsvdp_dma		
		; Compute DMA length
		movei.l	#$93009400, d0	; Set VDP command
		move.b	14(sp), d0	; Put MSB of len in VDP register 20
		swap	d0
		move.b	15(sp), d0	; Put LSB of len in VDP register 19

		; Send DMA length to VDP
		if _VDPREG
		move.l	d0, (a5)
		else
		move.l	d0, VDPCTRL
		endif

		; Compute DMA source address and copy mode
		lea	8(sp), a0
		roxr.w	(a0)		; Right-shift source address
		roxr.w	10(sp)		;
		move.b	#$97, (a0)	; Register 23 : source address MSB

		; Send source MSB to VDP
		if _VDPREG
		move.w	(a0), (a5)
		else
		move.w	(a0), VDPCTRL
		endif

		ori.l	#$95009600, d0	; Prepare VDP registers 21 and 22
		move.b	11(sp), d0	; Register 22 : source address middle byte
		swap	d0
		move.b	10(sp), d0	; Register 21 : source address LSB

		; Send DMA source address to VDP
		if _VDPREG
		move.l	d0, (a5)
		else
		move.l	d0, VDPCTRL
		endif
		
		; Compute DMA dest address
		lea	6(sp), a0
		move.w	(a0), d0
		andi.w	#$3FFF, d0
		ori.w	#$4000, d0

		; Send DMA dest address (first command)
		if _VDPREG
		move.w	d0, (a5)
		else
		move.w	d0, VDPCTRL
		endif

		rol.w	(a0)
		rol.w	(a0)
		andi.w	#$0003, (a0)
		ori.w	#$0080, (a0)

		; Send DMA dest address (second command)
		; Starts DMA transfer
		if _VDPREG
		move.w	d0, (a5)
		else
		move.w	d0, VDPCTRL
		endif

		rts

;;;;;
; Initialized prepared VDP buffer
; void blsvdp_prepare_init(short *prepared);
blsvdp_prepare_init
		move.l	4(sp), a0
		clr.w	(a0)
		rts

;;;;;
; Prepares VDP DMA transfer command in a VDP buffer.
; void blsvdp_prepare_dma(void *dest, void *src, unsigned int len, short *prepared);
blsvdp_prepare_dma
		move.l	16(sp), a0	; Load target pointer to a1
		move.w	(a0), d0
		lea	0(a0, d0), a1
		addi.w	#7, (a0)

		; Compute DMA length
		movei.l	#$93009400, d0	; Set VDP command
		move.b	14(sp), d0	; Put MSB of len in VDP register 20
		swap	d0
		move.b	15(sp), d0	; Put LSB of len in VDP register 19

		; Send DMA length to VDP
		move.l	d0, (a1)+

		; Compute DMA source address and copy mode
		lea	8(sp), a0
		roxr.w	(a0)		; Right-shift source address
		roxr.w	10(sp)		;
		move.b	#$97, (a0)	; Register 23 : source address MSB

		; Send source MSB to VDP
		move.w	(a0), (a1)+

		ori.l	#$95009600, d0	; Prepare VDP registers 21 and 22
		move.b	11(sp), d0	; Register 22 : source address middle byte
		swap	d0
		move.b	10(sp), d0	; Register 21 : source address LSB

		; Send DMA source address to VDP
		move.l	d0, (a1)+
		
		; Compute DMA dest address
		lea	6(sp), a0
		move.w	(a0), d0
		andi.w	#$3FFF, d0
		ori.w	#$4000, d0

		swap	d0		; Compute second DMA command
		move.w	(a0), d0
		rol.w	#2, d0
		andi.w	#$0003, d0
		ori.w	#$0080, d0

		; Send DMA dest address (second command)
		move.l	d0, (a1)+

		; Return pointer after data
		move.l	a1, d0

		rts

;;;;;
; Sends a prepared buffer to the VDP control port.
; Stream is terminated by $FFFF (all prepare functions add proper termination)
; void blsvdp_exec(short *prepared);
blsvdp_exec
		move.l	4(sp), a0
		if _VDPREG
		move	a5, a1
		else
		lea	VDPCTRL, a1
		endif

vdp_send	move.w	(a0)+, d0	; Read word count
		beq.b	vdp_send_finished

		move.w	(a0)+, (a1)	; Send data to VDP control port
		dbra	d0, vdp_send

vdp_send_finished

		rts
