        if ..UNDEF wscr_w
wscr_w          set     256
wscr_h          set     152
wscr_topline    set     19
wscr_botline    set     19 + wscr_h

                include bls_vdp.inc
                include bls_init.inc

wscr_init_vdp   macro   framebuffer_addr, plane_b_addr, sprites_addr, interlace

wscr_framebuffer_addr set framebuffer_addr

; Data structure returned by the routine, when returning the buffer to main
wscr_pad_status set     wscr_framebuffer_addr
wscr_pad1_status set    wscr_pad_status
wscr_pad2_status set    wscr_pad_status + 2
wscr_frame_count set    wscr_pad_status + 4


                pea             framebuffer_addr
                pea             .jump_regs\?
                jmp             _main_wscr_init_vdp
.reg_init_vdp\?
                db      VDPR00 | VDPHINT | VDPHVEN ; #00
                db      VDPR01 | VDPDMAEN       ; #01

                db      wscr_fbuf_plane >> 10 & $38 ; #02 - Plane A
                db      0                       ; #03 - Window
                db      plane_b_addr >> 13      ; #04 - Plane B
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

        endif

; vim: ts=8 sw=8 sts=8 et
