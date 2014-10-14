; Provides Genesis / SCD initializatoin routines.
; You must include bls_init.md in the same binary.

        if ..UNDEF bls_init_inc
bls_init_inc    set     1

        if BUS == BUS_MAIN
bls_init        macro   hint_handler, vint_handler
                ccall2  bls_init, hint_handler, vint_handler
                endm
        endif

VDPWIN_TOPLEFT  set     $0000
VDPWIN_TOPRIGHT set     $8000
VDPWIN_BOTLEFT  set     $0080
VDPWIN_BOTRIGHT set     $8080

bls_init_vdp    macro   hint, vint, plane_w, plane_h, plane_a_addr, window_addr, plane_b_addr, sprites_addr, hscroll_addr, hscroll_mode, vscroll_mode, window_x, window_y, window_pos, shadow, interlace
                bra.b   .jump_regs\?
.reg_init_vdp\?
        if hint
		db	VDPR00 | VDPHINT | VDPHVEN ; #00
        else
		db	VDPR00 | VDPHVEN ; #00
        endif
        if vint
		db	VDPR01 | VDPDISPEN | VDPVINT | VDPDMAEN ; #01
        else
		db	VDPR01 | VDPDISPEN | VDPDMAEN           ; #01
        endif
		db	plane_a_addr >> 10 & $38        ; #02 - Plane A
		db	window_addr >> 10 & $3E	; #03 - Window
		db	plane_b_addr >> 13   	; #04 - Plane B
		db	sprites_addr >> 9 & $7E ; #05 - Sprite attributes
		db	VDPR06			; #06
		db	VDPR07			; #07
		db	VDPR08			; #08
		db	VDPR09			; #09
		db	VDPR10			; #10
		db	VDPR11 | VDPEINT | hscroll_mode | vscroll_mode  ; #11
        if shadow
		db	VDPR12 | VDPH40 | VDPSTE | interlace	; #12
        else
		db	VDPR12 | VDPH40 | interlace	        ; #12
        endif
		db	hscroll_addr >> 10 & $3F                ; #13
		db	VDPR14			; #14
		db	VDPR15 | 2		; #15 - autoincrement
        if plane_w == 32
.VDPSCRH\?      set     VDPSCRH32
        elsif plane_w == 64
.VDPSCRH\?      set     VDPSCRH64
        else
.VDPSCRH\?      set     VDPSCRH128
        endif
        if plane_h == 32
.VDPSCRV\?      set     VDPSCRV32
        elsif plane_h == 64
.VDPSCRV\?      set     VDPSCRV64
        else
.VDPSCRV\?      set     VDPSCRV128
        endif
		db	VDPR16 | .VDPSCRH\? | .VDPSCRV\?	; #16 - Plane size
		db	(window_pos >> 8 & $FF) | window_x      ; #17
		db	(window_pos & $FF) | window_y           ; #18
		align	2
.jump_regs\?
                VDPSETALLREGS   .reg_init_vdp\?(pc)
                endm

bls_enable_interrupts macro
                andi    #$F8FF, SR
                endm

bls_disable_interrupts macro
                ori     #$0700, SR
                endm

        endif

; vim: ts=8 sw=8 sts=8 et
