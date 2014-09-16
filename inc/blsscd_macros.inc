SYNC_MAIN_SUB	macro

	if BUS == BUS_MAIN

        lea GA_COMMFLAGS + 1, a0
        bset    #4, -1(a0)
.1m\?
        btst    #4, (a0)
        beq.b   .1m\?
        bclr    #4, -1(a0)
.2m\?
        btst    #4, (a0)
        bne.b   .2m\?

	else

        lea GA_COMMFLAGS, a0
        bset    #4, 1(a0)
.1s\?
        btst    #4, (a0)
        beq.b   .1s\?
        bclr    #4, 1(a0)
.2s\?
        btst    #4, (a0)
        bne.b   .2s\?

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

.2\?
		endm

	endif

SUB_INTERRUPT	macro
		bset	#0, GA_RH
		endm

SUB_INT_ENABLE	macro
		move.b	#GA_IM_L1|GA_IM_L2|GA_IM_L3|GA_IM_L4|GA_IM_L5|GA_IM_L6, GA_IMASK + 1
		move	#$2000, SR
		endm

SUB_INT_DISABLE	macro
		move	#$2700, SR
		move.b	#0, GA_IMASK + 1
		endm

SET_INT_HANDLER	macro	vector, target
		movea.l	vector, a0
		move.w	#$4EF9, (a0)+
		move.l	#target, (a0)
		endm
