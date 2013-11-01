		include	bls.inc

IP_MAIN
		move.w	#$2700, sr		; Disable interrupts
		lea	(mainstack + mainstack_size).w, a7

		jsr	monitor_init		; Initialize monitor
		move.l  #G_INT_VBLANK, G_INTV_VBLANK
		move.w	#$2000, sr		; Enable interrupts
.1		bra.b	.1			; Monitor logic is in the vblank interrupt
