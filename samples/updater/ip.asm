		include	bls.inc

IP_MAIN
		move.w	#$2700, sr		; Disable interrupts
		jsr	bda_init		; Initialize monitor
.1		bra.b	.1			; Monitor logic is in the vblank interrupt
