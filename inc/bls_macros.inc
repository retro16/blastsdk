; Static memory copy
; TODO : optimize and choose algorithm based on size
; FIXME : only works on word aligned
BLS_COPY	macro	source, dest, size
		assert	dest + size <= source || dest >= source + size
		assert	!(source & 1)
		assert	!(dest & 1)
		assert	!(size & 1)

		movem.l	d0/a0/a1, -(a7)		; Store used registers
		lea	source.l, a0		; Source address for copy
		lea	dest.l, a1		; Dest address for copy
		move.w	#(size/2) - 1, d0
.copyloop
		move.w	(a0)+, (a1)+		; Copy to dest
		dbra	d0, .copyloop
		movem.l	(a7)+, d0/a0/a1		; Restore registers

		endm


; Relocate a symbol to its final location
; For now, only works on MAIN CPU with correct bank settings
BLS_RELOCATE	macro	symbol
	if SCD != 0
relocated\?	set	symbol ## _CONCAT
	else
relocated\?	set	symbol ## _ROM
	endif
	if	..UNDEF symbol || symbol != relocated\?		; Only relocate if needed

		BLS_COPY relocated\?, symbol, symbol ## _SIZE

	endif
		endm


BLS_INTTABLE_JUMP macro target

	if ..DEF int_##target
hwint_##target jmp int_##target.l
	elsif ..DEF interrupt
hwint_##target jmp interrupt.l
	else
hwint_##target jmp $200.l
	endif
intv_##target	set	hwint_##target + 2

		endm

BLS_INTTABLE_JUMP2 macro target, alttarget

	if ..DEF int_##target
hwint_##target jmp int_##target.l
	elsif ..DEF int_##alttarget
hwint_##target jmp int_##alttarget.l
	elsif ..DEF interrupt
hwint_##target jmp interrupt.l
	else
hwint_##target jmp $200.l
	endif
intv_##target	set	hwint_##target + 2

		endm

; Generate an interrupt jump table like the one on the SCD
; Genesis only (no effect for the SCD since it already has a jump table)
BLS_INTTABLE	macro

	if SCD == 0 && PGMCHIP == CHIP_CART

		bls_inttable_jump buserr
		bls_inttable_jump addrerr
		bls_inttable_jump ill
		bls_inttable_jump zdiv
		bls_inttable_jump chk
		bls_inttable_jump trapv
		bls_inttable_jump priv
		bls_inttable_jump linea
		bls_inttable_jump linef
		bls_inttable_jump spurious
		bls_inttable_jump2 level2, pad
		bls_inttable_jump2 level4, hblank
		bls_inttable_jump2 level6, vblank
		bls_inttable_jump trap00
		bls_inttable_jump trap01
		bls_inttable_jump trap02
		bls_inttable_jump trap03
		bls_inttable_jump trap04
		bls_inttable_jump trap05
		bls_inttable_jump trap06
		bls_inttable_jump trap08
		bls_inttable_jump trap09
		bls_inttable_jump trap10
		bls_inttable_jump trap11
		bls_inttable_jump trap12
		bls_inttable_jump trap13
		bls_inttable_jump trap14
		bls_inttable_jump trap15

	endif
		endm

BLS_EXCEPTION_TABLE	macro

	if SCD == 0 && PGMCHIP == CHIP_CART

		bls_inttable_jump buserr
		bls_inttable_jump addrerr
		bls_inttable_jump ill
		bls_inttable_jump zdiv
		bls_inttable_jump chk
		bls_inttable_jump trapv
		bls_inttable_jump priv
		bls_inttable_jump linea
		bls_inttable_jump linef
		bls_inttable_jump spurious

	endif
		endm

BLS_INTERRUPT_TABLE	macro

	if SCD == 0 && PGMCHIP == CHIP_CART

		bls_inttable_jump2 level2, pad
		bls_inttable_jump2 level4, hblank
		bls_inttable_jump2 level6, vblank

	endif
		endm

BLS_TRAP_TABLE	macro

	if SCD == 0 && PGMCHIP == CHIP_CART

		bls_inttable_jump trap00
		bls_inttable_jump trap01
		bls_inttable_jump trap02
		bls_inttable_jump trap03
		bls_inttable_jump trap04
		bls_inttable_jump trap05
		bls_inttable_jump trap06
		bls_inttable_jump trap08
		bls_inttable_jump trap09
		bls_inttable_jump trap10
		bls_inttable_jump trap11
		bls_inttable_jump trap12
		bls_inttable_jump trap13
		bls_inttable_jump trap14
		bls_inttable_jump trap15

	endif
		endm


bls__vectortarget macro target

		if ..DEF target
		dl	target
		elsif ..DEF interrupt
		dl	interrupt
		else
		dl	$200
		endif

		endm

bls__vectortarget2 macro target, alttarget

		if ..DEF target
		dl	target
		elsif ..DEF alttarget
		dl	alttarget
		elsif ..DEF interrupt
		dl	interrupt
		else
		dl	$200
		endif

		endm


; Set exception vectors.
; PAD, TRAP #07 and TRACE are not set to avoid conflicts with BDA
; Alters CCR, d0, d1, d2, a0, a1, a2
BLS_SETVECTORS	macro
	if SCD != 0
		lea	.vectorlist, a0
		lea	.vectortargets, a1
		move.w	#(.vectorlist_end-.vectorlist)-1, d0
		moveq	#0, d1			; Reset high order bytes
.vector
		move.b	(a0)+, d1		; Load vector address into d1
		move.l	d1, a2
		move.l	(a2), a2		; Load vector target to a2
		move.l	(a1)+, 2(a2)		; Write new vector target after jmp instruction
		dbra	d0, .vector
		bra.w	.setvector_end

.vectorlist
	if SCD == 0
		hex	0810607078
	endif
		hex	0C14181C20282C
		hex	8084888C909498A0A4A8ACB0B4B8BC
.vectorlist_end
		align	2

.vectortargets
	if SCD == 0
		bls__vectortarget int_buserr
		bls__vectortarget int_ill
		bls__vectortarget int_spurious
		bls__vectortarget2 int_level4, int_hblank
		bls__vectortarget2 int_level6, int_vblank
	endif

		bls__vectortarget int_addrerr
		bls__vectortarget int_zdiv
		bls__vectortarget int_chk
		bls__vectortarget int_trapv
		bls__vectortarget int_priv
		bls__vectortarget int_linea
		bls__vectortarget int_linef

		bls__vectortarget int_trap00
		bls__vectortarget int_trap01
		bls__vectortarget int_trap02
		bls__vectortarget int_trap03
		bls__vectortarget int_trap04
		bls__vectortarget int_trap05
		bls__vectortarget int_trap06
		bls__vectortarget int_trap08
		bls__vectortarget int_trap09
		bls__vectortarget int_trap10
		bls__vectortarget int_trap11
		bls__vectortarget int_trap12
		bls__vectortarget int_trap13
		bls__vectortarget int_trap14
		bls__vectortarget int_trap15
.setvector_end
	endif
		endm

