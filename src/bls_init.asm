; Initializes the Genesis in a useable state.
; Synchronizes with the Sega CD

; void bls_init(bls_int_callback hint_handler, bls_int_callback vint_handler);
bls_init
        if BUS == BUS_MAIN
        ; Initialize Genesis part

        VDPSECURITY                     ; Unlock TMSS

        movem.l a4-a5, -(sp)
        VDPSETADDRREG                   ; Use a4-a5 for VDP access

        VDPENABLE 0, 0, 0, 1            ; Enable DMA only
        VDPSETAUTOINCR 2                ; VDPDMAFILL requires 2
        VDPDMAFILL 0, 0, $10000         ; Start clearing VRAM

        move.b	#CSEL, CCTRL1		; Enable CSEL for gamepad 1
        move.b  #CSEL, CCTRL2           ; Enable CSEL for gamepad 2

        jsr     bda_init                ; Init BDA
        beh_init                        ; Init BEH

        if TARGET == TARGET_SCD1 || TARGET == TARGET_SCD2
        move.l  INTV_VBLANK.w, a0
        move.w  #$4EF9, (a0)+
        move.l  8(sp), (a0)
        move.w  14(sp), GA_HINT
        endif

        VDPDMAWAIT                      ; Wait for VRAM cleaning

        VDPSETWRITE 0, CRAM             ; Clear CRAM
        moveq   #0, d0
        moveq   #31, d1                 ; 64 colors to clear
.clrpal move.l  d0, (a4)                ; Clear 2 colors
        dbra    d1, .clrpal

        VDPSETWRITE 0, VSRAM            ; Clear VSRAM
        moveq   #19, d1                 ; 40 words to clear
.clrvs  move.l  d0, (a4)                ; Clear 2 words
        dbra    d1, .clrvs

        VDPUSEADDR
        movem.l (sp)+, a4-a5

        if TARGET == TARGET_SCD1 || TARGET == TARGET_SCD2
        SYNC_MAIN_SUB
        endif

        else
        assert  BUS == BUS_SUB
        ; Initialize Sega CD part

        ; Enable interrupts
        move.w  #GA_IM_L1 | GA_IM_L2 | GA_IM_L3 | GA_IM_L4 | GA_IM_L5 | GA_IM_L6, GA_IMASK

        if TARGET == TARGET_SCD1 || TARGET == TARGET_SCD2
        SYNC_MAIN_SUB
        endif

        endif

        rts

; vim: ts=8 sw=8 sts=8 et
