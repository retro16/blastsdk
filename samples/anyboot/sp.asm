		include	bls.inc
		include	cdbios.inc

cdheader	equ	$00FE00			; Contains new IP code


sp_init
sp_main
		BIOS_CDCSTOP
		lea	sp_track(pc), a0
		BIOS_DRVINIT
		
		SYNC_MAIN_SUB			; Wait until user pressed START

		lea	sp_track(pc), a0	; Re-init CD drive
		BIOS_DRVINIT

		; Read IP from CD
		moveq	#0, d0
		moveq	#17, d0			; Read header + IP (32k)
		lea	cdheader, a0
		jsr	ReadCD

		; Copy code into COPY_CODE and run it
		lea	target_code, a0
		lea	copy_code, a1
		move.w	#(copy_code_end-copy_code) / 2 - 1, d0
.1		move.w	(a1)+, (a0)+
		dbra	d0, .1
		jmp	target_code

sp_track	dc.b	$01, $FF		; TOC in track 1, read all tracks

int_sub_level2
		rte

		include	blsscd_routines.inc

copy_code
		rorg	$05F000
target_code
		; Find SP location and read it from CD
		move.l	cdheader + $40, d0
		lsr.l	#8, d0			; Convert offset to sector
		lsr.l	#3, d0
		move.l	cdheader + $44, d1	; SP size
		lsr.l	#8, d1			; Convert size to sector
		lsr.l	#3, d1
		addq.l	#1, d1			; Overread for security
		lea	$6000, a0		; Read to final RAM address
		jsr	ReadCD

		; Find entry points
		move.l	$6018, d0		; Entry point table offset
		lea	$6000, a0		; Header offset
		lea	(a0, d0), a1		; Entry points base address
		move.l	a1, a2			; Entry points pointer

		; SP_INIT
		move.w	(a2)+, d1
		lea	(a1, d1), a3
		move.w	#$4EB9, (a0)+		; Generate JSR
		move.l	a3, (a0)+
		move.l	a3, $5F2A		; BIOS jump table

		; SP_MAIN
		move.w	(a2)+, d1
		lea	(a1, d1), a3
		move.w	#$4EB9, (a0)+		; Generate JSR
		move.l	a3, (a0)+
		move.l	a3, $5F30		; BIOS jump table

		; Level 2 interrupt
		move.w	(a2)+, d1
		lea	(a1, d1), a3
		move.l	a3, $0068
		move.l	a3, $5F36		; BIOS jump table

		; Other subprograms
		move.w	(a2)+, d1
		beq.b	.1			; End of list
		lea	(a1, d1), a3
		move.w	#$4EB9, (a0)+		; Generate JSR
		move.l	a3, (a0)+

		; Set reset program (overwrite header)
.1		move.l	#$6000, $000004.w

		SYNC_MAIN_SUB			; Signal main that we are ready
wait_reset	bra.b	wait_reset		; Wait until main resets sub

		rend
copy_code_end

