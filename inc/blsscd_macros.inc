SYNC_MAIN_SUB	macro

	if BUS == BUS_MAIN

		move.b	#1, GA_COMMOUT
.1m\?		tst.b	GA_COMMIN
		beq.b	.1m\?

		move.b	#0, GA_COMMOUT
.2m\?		tst.b	GA_COMMIN
		beq.b	.2m\?

	else

.1s\?		tst.b	GA_COMMIN
		beq.b	.1s\?
		move.b	#1, GA_COMMOUT

.2s\?		tst.b	GA_COMMIN
		bne.b	.2s\?
		move.b	#0, GA_COMMOUT

	endif

		endm


	if BUS == BUS_MAIN

SUB_BUSREQ	macro
		move.w	#(GA_NORESET | GA_BUSREQ), GA_RH
		endm

SUB_BUSRELEASE	macro
		move.w	#GA_NORESET, GA_RH
		endm

SUB_RESET	macro
		move.w	#GA_NORESET, GA_RH
		move.w	#0, GA_RH
		move.w	#GA_NORESET, GA_RH
		endm

SUB_ACCESS_BANK	macro b
		move.w	d0, -(sp)

		; Check current mode
		move.w	GA_MM, d0
		btst	#2, d0
		beq	.1\?
		btst	#1, d0
		beq	.1\?

		; 2M mode with memory on sub : preserve DMNA bit
		move.w	#(b<<6 | 2), GA_MM
		bra	.2\?

		; Other modes : leave 0 in DMNA to avoid bank switch
.1\?		move.w	#(b<<6), GA_MM

.2\?		move.w	(sp)+, d0
		endm

	endif
