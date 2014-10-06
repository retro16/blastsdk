SP_MAIN
		; Enable interrupts
		move.w	#$2000, sr
		move.w	GA_IMASK, INTMASK.w
		move.w	#GA_IM_L1 | GA_IM_L2 | GA_IM_L3 | GA_IM_L4 | GA_IM_L5 | GA_IM_L6, GA_IMASK

		; Start
		moveq	#0, d0
.loop
		addq.l	#1, d0
		move.l	d0, INDICATOR.w
		bra.b	.loop

INDICATOR		dl		0
		hex		DEADBEEF
INTMASK		dw		$FFFF

SUB_INT_LEVEL2
		rts
