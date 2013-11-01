		include	bls.inc
ip_main
		jsr	bda_init
		move.w	#CSEL, CCTRL1	; Enable select line on pad 1
		move.w	#CSEL, CCTRL1	; Select A/Start line
		nop

		; Wait until pressed START
.1		btst	#BCBTNSTART, CDATA1
		beq.b	.1
		move.w	#0, CCTRL1	; Release pad select line

		SYNC_MAIN_SUB		; Sync with sub
		SYNC_MAIN_SUB		; Second sync when data is available

		; Copy copy program at the end of RAM
		lea	target_code, a0
		lea	copy_code, a1
		move.w	#(copy_code_end-copy_code) / 2 - 1, d0
.2		move.w	(a1)+, (a0)+
		dbra	d0, .2

		jmp	target_code

copy_code
		rorg	$FF8000
target_code
		; This code is reallocated at the end of RAM
		
		; Read from sub CPU RAM
		SUB_BUSREQ
		SUB_ACCESS_BANK	0

		; Copy sub CPU program in main RAM
		lea	$FF0000, a0
		lea	$030000, a1
		move.w	#$3FFF, d0		; Reproduce buggy BIOS behaviour by
.1		move.w	(a1)+, (a0)+		; copying the first $8000 bytes of
		dbra	d0, .1			; IP code to main RAM

		; Detect region and compute entry point
detect_region	move.b	$FF0011, d0
		cmp.b	#$7a, d0
		bne.b	.1
		lea	$FF0584, a0		; US game entry point
		bra.b	.3
.1		cmp.b	#$64, d0
		bne.b	.2
		lea	$FF056E, a0		; EU game entry point
		bra.b	.3
.2		lea	$FF0156, a0		; JP game entry point
.3

		; Release busreq and reset sub CPU
		SUB_RESET

		; Start new IP
		jmp	(a0)

		rend
copy_code_end

