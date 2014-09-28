; Provides Genesis / SCD initializatoin routines.
; You must add bls_init.asm in the same binary.

bls_init        macro   hint_handler, vint_handler
                ccall2  bls_init hint_handler, vint_handler
                endm

VDPWIN_TOPLEFT  set     $0000
VDPWIN_TOPRIGHT set     $8000
VDPWIN_BOTLEFT  set     $0080
VDPWIN_BOTRIGHT set     $8080

bls_init_vdp    macro   hint, vint, plane_w, plane_h, plane_a_addr, window_addr, plane_b_addr, sprites_addr, hscroll_addr, hscroll_mode, vscroll_mode, window_x, window_y, window_pos, shadow, interlace
                VDPSETALLREGS   reg_init_vdp\?(pc)
                bra.b   .jump_regs\?
reg_init_vdp\?
		db	VDPR00 | VDPHVEN | (hint ? VDPHINT : 0) ; #00
		db	VDPR01 | VDPDISPEN | (vint ? VDPVINT : 0) | VDPDMAEN	; #01
		db	plane_a_addr >> 10 & $38        ; #02 - Plane A
		db	window_addr >> 10 & $3E	; #03 - Window
		db	plane_b_addr >> 13   	; #04 - Plane B
		db	sprites_addr >> 9 & $7E ; #05 - Sprite attributes
		db	VDPR06			; #06
		db	VDPR07			; #07
		db	VDPR08			; #08
		db	VDPR09			; #09
		db	VDPR10			; #10
		db	VDPR11 | VDPEINT | hscroll_mode | vscroll_mode
		db	VDPR12 | VDPH40 | (shadow?VDPSTE:0) | interlace	; #12
		db	hscroll_addr >> 10 & $3F        ; #13
		db	VDPR14			; #14
		db	VDPR15 | 2		; #15 - autoincrement
		db	VDPR16 | (plane_w==32 ? VDPSCRH32 : plane_w==64 ? VDPSCRH64 : VDPSCRH128) | (plane_h==32 ? VDPSCRV32 : plane_h==64 ? VDPSCRV64 : VDPSCRV128)	; #16 - Plane size
		db	(window_pos >> 8 & $FF) | window_x      ; #17
		db	(window_pos & $FF) | window_y           ; #18
		align	2
.jump_regs\?
                endm

bls_enable_interrupts macro
                andi    #$F8FF, SR
                endm

bls_disable_interrupts macro
                ori     #$0700, SR
                endm

; vim: ts=8 sw=8 sts=8 et
