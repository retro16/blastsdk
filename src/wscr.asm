                include bls_vdp.inc

_main_wscr_init_vdp
                ; TODO
                ; Create the plane pattern in vram
                ; 
                ; Example for 256x152 screen:
                ;
                ;  00  19  38      551 570 589
                ;  01  20              571 590
                ;  02                      591
                ;                             
                ;  16                      605
                ;  17  36              587 606
                ;  18  37  56      569 588 607

                movem.l d5-d7/a4-a5, -(sp)
                move.l  24(sp), d7              ; Read parameter to d7
                
                VDPSETADDRREG
                VDPSETAUTOINCR 2
                VDPSETWRITE wscr_fbuf_plane, VRAM

                ; d1 contains line countdown
                ; d6 contains cell count per column
                moveq   #wscr_h / 8, d1
                move.w  d1, d6

                ; d5 contains empty cell data
                move.w  d7, d5
                ori.w   #wscr_fbuf_chr / $20 + wscr_w * wscr_h / 64, d5

                ; d7 contains current cell data
                ori.w   #wscr_fbuf_chr / $20, d7


.gen_line       ; Generate one line of plane data

                ; Left border
                moveq   #(320-wscr_w)/8/2, d0
.left_border    VDPWRITEW d5
                dbhi    d0, .left_border

                ; Generate one line of framebuffer
                moveq   #wscr_w/8, d0
.gen_chr        VDPWRITEW d7
                add.w   d6, d7
                dbhi    d0, .gen_chr

                ; Compute offset of the first cell of next line
                subi.w  #(wscr_w * wscr_h / 64) - 1, d7

                ; Right border
                moveq   #64 - ((320-wscr_w)/8/2), d0
.right_border   VDPWRITEW d5
                dbhi    d0, .right_border

                dbhi    d1, .gen_line

                ; Reset horizontal scrolling
                VDPSETWRITE wscr_hscroll, VRAM
                VDPWRITEL d1                    ; d1.l == 0 at this point

                ; Reset vertical scrolling
                VDPSETWRITE 0, VSRAM
                VDPWRITEL #wscr_topline << 16 | wscr_topline ; The 2 planes must be offset by wscr_topline lines

                VDPUSEADDR
                movem.l (sp)+, d5-d7/a4-a5
                rts

_main_wscr_hblank
                VDPSETREG 1, VDPR01 | VDPDMAEN  ; Disable display
                ori     #$0600, sr

 VDPSETBG #$400
                HAS_WRAM_2M
                bne.w   wscr_no_frame_available ; Oops, the sub CPU did not have time to render !

                movem.l d0-d1/a0-a1/a4-a5, -(sp)
                VDPSETADDRREG

                ; Patched by JSR <addr>.l by wscr_init_vdp if there is a vblank callback
wscr_vblank_callback
                moveq   #$01, d0
                moveq   #$01, d0
                moveq   #$01, d0
                ; End of patch

                ; Upload the framebuffer to VRAM using DMA
                tst.w   d0
                beq.b   .no_refresh
                VDPDMASEND wscr_framebuffer_addr, wscr_fbuf_chr, wscr_w * wscr_h / 2, VRAM
.no_refresh

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
                lea     GA_COMMOUT, a1
                move.l  (a0), (a1)

                ; Switch SEL line on gamepads
        if BDACTRL != CCTRL1
                bclr    #CSEL_BIT, CCTRL1
        endif
        if BDACTRL != CCTRL2
                bclr    #CSEL_BIT, CCTRL1
        endif
                ; Update missed frame counter while pads are stabilizing
                move.w  wscr_miss_count(pc), 4(a1)
                clr.w   wscr_miss_count(pc)

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

                st.b    wscr_vbl_flag(pc)       ; Set VBL flag

 VDPSETBG #$000
                movem.l (sp)+, d0-d1/a0-a1/a4-a5
                rte
                
wscr_no_frame_available
                addi.w  #1, wscr_miss_count    ; Count missed frames
 VDPSETBG #$004
                VDPWAITLINE wscr_topline        ; Wait until the end of extended vblank
                VDPSETREG 1, VDPR01 | VDPDISPEN | VDPDMAEN ; Enable display
                rte

wscr_miss_count dw      0
wscr_vbl_flag   db      0
                db      0


; vim: ts=8 sw=8 sts=8 et
