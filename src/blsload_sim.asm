; Injected by bdb into the sub cpu bios.

; Compile with :
;  asmx -w -e -b 0xA02 -C 68000 -l /dev/stdout -o /tmp/blsload_sim.bin blsload_sim.asm -I ../include -i bls.inc -d BUS:=2 -d CHIP:=9 -d TARGET:=2

; Implemented calls :
; DRVINIT
; CDCSTAT
; CDCSTOP
; CDCACK
; ROMREAD
; CDCREAD
; CDCTRN

BDP_OUT_BUFSIZE equ     $30     ; Address of data size in buffer
BDP_OUT_BUFFER  equ     $32     ; Address of sub buffer
BDP_OUT_MAXSIZE equ     $2E     ; Max number of bytes in buffer

COMBUF	set		$200	; Communication buffer
COMFLAG	set		$A00	; Communication flag (0 if COMBUF empty, any other value when COMBUF full)

		org		$A02
BLSLOAD_SIM_ENTRY
		cmp.b	#$8A, d0
		beq.b	CDCSTAT

		cmp.b	#$89, d0
		beq.b	CDCSTOP

		cmp.b	#$8D, d0
		beq.b	CDCACK

		cmp.b	#$17, d0
		beq.b	ROMREAD

		cmp.b	#$8B, d0
		beq.b	CDCREAD

		cmp.b	#$8C, d0
		beq.b	CDCTRN

		rts

CDCSTAT
		tst.b	COMFLAG.w
		bne.b	.full
		move	#1, CCR
.full
		rts

CDCSTOP
CDCACK
		clr.b	COMFLAG.w
		rts

ROMREAD
		movem.l	a0/a1, -(sp)
		lea		5(a0), a0
		lea		BDP_OUT_BUFFER.w, a1
		move.b	(a0)+, (a1)+
		move.b	(a0)+, (a1)+
		move.b	(a0)+, (a1)+
		jsr		BDP_WRITE_BUF(PC)
		movem.l	(sp)+, a0/a1
		rts

; Sends 3 bytes in BDP_WRITE_BUF using BDP
BDP_WRITE_BUF
		move.w	#3, BDP_OUT_BUFSIZE.w
		bset	#5, GA_COMMFLAGS_SUB.w
.wait_flush
		tst.w   BDP_OUT_BUFSIZE
		bne.b   .wait_flush
		bclr    #5, GA_COMMFLAGS_SUB.w
		rts

CDCREAD
		movem.l	a0/a1, -(sp)
		move.l	#$0FFFFF00, BDP_OUT_BUFFER.w
		jsr		BDP_WRITE_BUF(PC)
		movem.l	(sp)+, a0/a1
		rts

CDCTRN
		movem.l	d0/a1, -(sp)
		lea		COMBUF.w, a1
		move.w	#$1FF, d0
.trn_copy
		move.l	(a1)+, (a0)+
		dbra	d0, .trn_copy
		movem.l	(sp)+, d0/a1
		addq	#4, a1
		rts


