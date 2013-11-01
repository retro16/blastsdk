		include bls.inc
		VDPSECURITY			; Unlock TMSS
	if PGMCHIP == CHIP_CART
		bls_relocate inttable_asm
	endif
		bsr.b	monitor_init		; Initialize monitor
		move.w	#$2000, sr		; Enable interrupts
.1		bra.b	.1			; Monitor logic is in the vblank interrupt

