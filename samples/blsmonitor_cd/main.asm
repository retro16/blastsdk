		include bls.inc
		include bda.inc
IP_MAIN
MAIN
		VDPSECURITY			; Unlock TMSS
		jsr	bda_init		; Initialize debug agent
		jsr	monitor_init		; Initialize monitor
		move.w	#$2000, sr		; Enable interrupts
.1		bra.b	.1			; Monitor logic is in the vblank interrupt
