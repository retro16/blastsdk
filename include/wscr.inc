        if ..UNDEF wscr_w
wscr_w          set     256
wscr_h          set     152
wscr_topline    set     16
wscr_botline    set     wscr_topline + wscr_h

wscr_pad_up     set     $0001
wscr_pad_down   set     $0002
wscr_pad_left   set     $0004
wscr_pad_right  set     $0008
wscr_pad_a      set     $1000
wscr_pad_b      set     $0010
wscr_pad_c      set     $0020
wscr_pad_start  set     $2000

wscr_pad_up_bit     set     0
wscr_pad_down_bit   set     1
wscr_pad_left_bit   set     2
wscr_pad_right_bit  set     3
wscr_pad_a_bit      set    12
wscr_pad_b_bit      set     4
wscr_pad_c_bit      set     5
wscr_pad_start_bit  set    13

wscr_pad_status set     GA_COMMOUT
wscr_pad1_status set    wscr_pad_status
wscr_pad2_status set    wscr_pad_status + 2
wscr_miss_count set     wscr_pad_status + 4

                include bls_vdp.inc
                include bls_init.inc

wscr_init_vdp   macro   vblank_callback, vblank_miss_callback, framebuffer_addr, tile_attr, plane_a_addr, sprites_addr, interlace

wscr_framebuffer_addr set framebuffer_addr

        if vblank_callback
                move.w  #$4EB9, wscr_vblank_callback ; JSR.L
                move.l  #vblank_callback, wscr_vblank_callback + 2
        endif
        if vblank_miss_callback
                move.w  #$4EB9, wscr_vblank_miss_callback ; JSR.L
                move.l  #vblank_miss_callback, wscr_vblank_miss_callback + 2
        endif

                pea     tileattr
                pea     .jump_regs\?            ; Function returns after registers
                jmp     _main_wscr_init_vdp
.reg_init_vdp\?
                db      VDPR00 | VDPHINT | VDPHVEN ; #00
                db      VDPR01 | VDPDMAEN       ; #01

                db      plane_a_addr>>10 & $38  ; #02 - Plane A
                db      0                       ; #03 - Window
                db      wscr_fbuf_plane >> 13   ; #04 - Plane B
                db      sprites_addr >> 9 & $7E ; #05 - Sprite attributes
                db      VDPR06                  ; #06
                db      VDPR07                  ; #07
                db      VDPR08                  ; #08
                db      VDPR09                  ; #09
                db      VDPR10 | wscr_botline   ; #10 - hblank period
                db      VDPR11 | VDPEINT        ; #11
                db      VDPR12 | VDPH40 | interlace ; #12
                db      wscr_hscroll >> 10 & $3F ; #13
                db      VDPR14                  ; #14
                db      VDPR15 | 2              ; #15 - autoincrement
                db      VDPR16 | VDPSCRH64 | VDPSCRV32 ; #16 - Plane size
                db      0                       ; #17
                db      0                       ; #18
        align   2
.jump_regs\?
                VDPSETALLREGS   .reg_init_vdp\?(pc)
                endm


                ; Disable widescreen and go back to normal screen
                ; Warning: do not re-enable hblank interrupt
wscr_hold       macro
                VDPSETREG 0, VDPR00 | VDPHVEN
                endm

                ; Resume widescreen display
wscr_resume     macro
                VDPSETREG 0, VDPR00 | VDPHINT | VDPHVEN
                endm


        endif

; vim: ts=8 sw=8 sts=8 et
