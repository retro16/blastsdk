main
		VDPSECURITY

		; Use a5 and a6 registers for VDP address
		VDPSETADDRREG			; Use a5/a6 for VDP macros

		VDPSETALLREGS reg_init_vdp	; Load reg_init_vdp to VDP registers
		VDPDMAFILL #$00, 0, $10000	; Clear VRAM
		VDPDMAWAIT
		VDPSETAUTOINCR	2
		BLS_LOAD_BINARY_MAIN

		move.b	#CSEL, CCTRL1		; Setup select line on controller

		lea	Hello_World, a0
		bsr.b	print

		trap	#00
		move.w	#$2000, sr		; Enable interrupts
.1		bra.b	.1

; Print string pointed by a0
; Destroys d0
print
		moveq	#0, d0
		VDPSETWRITEPLANE PLANE_A, 64, 0, 0
.1		move.b	(a0)+, d0
		beq.b	.2
		move.w	d0, (a4)
		bra.b	.1
.2		rts

Hello_World	ASCIIZ	"Hello Blast ! World"


reg_init_vdp
		db	VDPR00			; #00
		db	VDPR01 | VDPDISPEN | VDPVINT | VDPDMAEN	; #01
		db	VDPR02			; #02 - Plane A
		db	VDPR03			; #03 - Window
		db	VDPR04			; #04 - Plane B
		db	VDPR05			; #05 - Sprite attributes
		db	VDPR06			; #06
		db	VDPR07			; #07
		db	VDPR08			; #08
		db	VDPR09			; #09
		db	VDPR10			; #10
		db	VDPR11 | VDPEINT	; #11
		db	VDPR12 | VDPH40		; #12
		db	VDPR13			; #13
		db	VDPR14			; #14
		db	VDPR15 | 1		; #15 - autoincrement
		db	VDPR16 | VDPSCRH64	; #16
		db	VDPR17			; #17
		db	VDPR18			; #18

