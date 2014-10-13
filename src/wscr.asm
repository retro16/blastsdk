                include bls_vdp.inc

_main_wscr_init_vdp
                ; TODO
                ; Create the plane pattern in vram

                ; Reset scrolling

                rts

_main_wscr_hblank
                VDPSETREG 1, VDPR01 | VDPDMAEN  ; Disable display
                ori     #$0600, sr

 VDPSETBG #$400
                HAS_WRAM_2M
                bne.w   .no_frame_available     ; Oops, the sub CPU did not have time !

                movem.l d0-d1/a0-a1/a4-a5, -(sp)
                VDPUSEREG

                ; Upload the framebuffer to VRAM using DMA
                VDPDMASEND wscr_framebuffer_addr, wscr_fbuf_chr, wscr_w * wscr_h / 2, VRAM

                ; Wait for the correct display line and reenable display
                move.w  VDPHV, -(sp)            ; Query current line
                tst.b   (sp)                    ; Test if negative
                addq.l  #2, (sp)
                bgt.b   .tophalf                ; line is in the top half of the screen
                VDPWAITLINE wscr_topline
                bra.b   .enable_display
.tophalf        VDPUNTILLINE wscr_topline
.enable_display
                VDPSETREG 1, VDPR01 | VDPDISPEN | VDPDMAEN ; Enable display

                ; Poll gamepads
                lea     CDATA1 - 1, a0
                lea     wscr_pad_status, a1
                move.l  (a0), (a1)

                ; Switch SEL line on gamepads
        if BDACTRL != CCTRL1
                bclr    #CSEL_BIT, CCTRL1
        endif
        if BDACTRL != CCTRL2
                bclr    #CSEL_BIT, CCTRL1
        endif
                ; Update frame counter while pads are stabilizing
                move.w  .frame_count(pc), 4(a1)
                clr.w   .frame_count(pc)

                ; Poll second half of gamepads
                movep.w 1(a0), d0
                movep.w d0, (a1)

                ; Reset SEL line for next frame
        if BDACTRL != CCTRL1
                bclr    #CSEL_BIT, CCTRL1
        endif
        if BDACTRL != CCTRL2
                bclr    #CSEL_BIT, CCTRL1
        endif

                ; Give memory back to sub CPU
                SEND_WRAM_2M

                ; Generate the VBL interrupt for sub CPU
                SUB_INTERRUPT

 VDPSETBG #$000
                movem.l (sp)+, d0-d1/a0-a1/a4-a5
                rte
                
.no_frame_available
                addi.w  #1, .frame_count        ; Count missed frames
 VDPSETBG #$004
                VDPWAITLINE wscr_topline        ; Wait until the end of extended vblank
                VDPSETREG 1, VDPR01 | VDPDISPEN | VDPDMAEN ; Enable display
                rte

.frame_count    dw      0


; vim: ts=8 sw=8 sts=8 et
