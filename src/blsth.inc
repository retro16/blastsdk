_blsth_init	macro	ptr
		pea	ptr
		jsr	blsth_init
		lea	-4(sp), sp
		endm

_blsth_run	macro	ptr
		pea	ptr
		jsr	blsth_run
		lea	-4(sp), sp
		endm

_blsth_yield	macro
		jsr	blsth_yield
		endm

_blsth_call	macro	ptr, fct
		move.l	sp, blsth_sp
		pea	fct
		pea	ptr
		jsr	blsth_pushcall
		lea	-8(sp), sp
		endm

_blsth_call_w	macro	ptr, fct, p1
		move.l	sp, blsth_sp
		move.w	p1, -(sp)
		pea	fct
		pea	ptr
		jsr	blsth_pushcall
		lea	-10(sp), sp
		endm

_blsth_call_l	macro	ptr, fct, p1
		move.l	sp, blsth_sp
		pea	p1
		pea	fct
		pea	ptr
		jsr	blsth_pushcall
		lea	-12(sp), sp
		endm

_blsth_call_ww	macro	ptr, fct, p1, p2
		move.l	sp, blsth_sp
		pea	p2
		pea	p1
		pea	fct
		pea	ptr
		jsr	blsth_pushcall
		lea	-12(sp), sp
		endm

_blsth_call_wl	macro	ptr, fct, p1, p2
		move.l	sp, blsth_sp
		pea	p2
		move.w	p1, -(sp)
		pea	fct
		pea	ptr
		jsr	blsth_pushcall
		lea	-14(sp), sp
		endm

_blsth_call_lw	macro	ptr, fct, p1, p2
		move.l	sp, blsth_sp
		move.w	p2, -(sp)
		pea	p1
		pea	fct
		pea	ptr
		jsr	blsth_pushcall
		lea	-14(sp), sp
		endm

_blsth_call_ll	macro	ptr, fct, p1, p2
		move.l	sp, blsth_sp
		pea	p2
		pea	p1
		pea	fct
		pea	ptr
		jsr	blsth_pushcall
		lea	-16(sp), sp
		endm

