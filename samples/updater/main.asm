		include bls.inc
		VDPSECURITY			; Unlock TMSS
		bsr.w	monitor_init		; Initialize monitor
.1		bra.b	.1			; Monitor logic is in the vblank interrupt

