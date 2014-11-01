        if ..UNDEF BLS_NBEH
                if TARGET == TARGET_GEN
int_buserr      move.w  #$0008, -(a7)
                bra.b   beh_display_exception
                endif
int_addrerr     move.w  #$000C, -(a7)
                bra.b   beh_display_exception
                if TARGET == TARGET_GEN
int_ill         move.w  #$0010, -(a7)
                bra.b   beh_display_exception
                endif
int_zdiv        move.w  #$0014, -(a7)
                bra.b   beh_display_exception
int_chk         move.w  #$0018, -(a7)
                bra.b   beh_display_exception
int_trapv       move.w  #$001C, -(a7)
                bra.b   beh_display_exception
int_priv        move.w  #$0020, -(a7)
                bra.b   beh_display_exception
int_linea       move.w  #$0028, -(a7)
                bra.b   beh_display_exception
int_linef       move.w  #$002C, -(a7)
                bra.b   beh_display_exception
                if TARGET == TARGET_GEN
int_spurious    move.w  #$0060, -(a7)
                bra.b   beh_display_exception
                endif
int_trap00      move.w  #$0080, -(a7)

; Displays the stack, one element per line
; Contains in order D0-D7, A0-A7, vector, exception stack frame, top of program stack
beh_display_exception
                move.w  #$2700, sr              ; Disable exceptions
                subq    #8, a7                  ; Reserve space for SP+USP
                movem.l d0-d7/a0-a6, -(a7)      ; Push registers to be displayed
                movem.l d0-d7/a0-a6, bda_d0.w   ; Store registers to BDA
                lea     70(a7), a0              ; Compute SSP value before pushing registers
                move.l  a0, 60(a7)              ; Move SSP in previously reserved space
                move.l  a0, bda_sp.w            ; Store SSP to BDA
                move    usp, a0                 ; Get USP
                move.l  a0, 64(a7)              ; Move USP in previously reserved space

                move.l  72(a7), bda_pc.w        ; Copy PC to BDA RAM
                move.w  70(a7), bda_sr.w        ; Copy SR to BDA RAM

                moveq   #0, d0
                move.w  68(a7), d0              ; Read exception vector
                lsr.w   #2, d0                  ; Compute exception identifier
                move.l  d0, bda_pkt_header.w    ; Store exception identifier for bda

                VDPSETADDRREG                   ; Set a4 and a5 to data and control registers resp.

                VDPSETALLREGS beh_vdp_regs

                VDPSETWRITE 0, VRAM

                ; Unpack font into the beginning of VRAM
                move.w  #(beh_font_end-beh_font)-1, d0
                lea     beh_font(pc), a0
.unpackbyte
                move.b  (a0)+, d1               ; d1 = packed data (1BPP)
                moveq   #8, d2
.expandchar     lsl.l   #3, d3                  ; d3 = unpacked data (4BPP)
                lsl.b   #1, d1
                roxl.l  #1, d3
                dbra    d2, .expandchar
                move.l  d3, (a4)
                dbra    d0, .unpackbyte

                ; Clear the rest of VRAM
                move.w  #$3DDF, d0
                clr.l   d3                      ; d3 = 0 from now on
.vramclear      move.l  d3, (a4)
                dbra    d0, .vramclear

                VDPSETWRITE 0, CRAM
                move.l  #$00000AAA, (a4)        ; Set background to black and text to grey

                VDPSETWRITE 0, VSRAM
                move.l  d3, (a4)                ; Clear vertical scroll

                moveq   #1, d4                  ; Column count
                moveq   #18, d7                 ; Line count
                move.w  #PLANE_A_DEF+64*2*2+12, d6 ; VRAM address of the beginning of the 3rd line

.1              
                VDPSETWRITEVAR d6, VRAM
                move.l  (a7)+, d0               ; d0 = value to display
                moveq   #7, d5                  ; 8 nybbles to display
                move.l  d3, (a4)                ; 2 spaces before data
.2              rol.l   #4, d0                  ; Select nybble
                move.w  d0, d1                  ; d1 = plane entry
                andi.w  #$000F, d1
                addq.b  #1, d1
                move.w  d1, (a4)                ; Display nybble
                dbra    d5, .2
                move.l  d3, (a4)                ; 2 spaces after data
.3              addi.w  #64*2, d6               ; Go to next line
                cmp.w   #11, d7                 ; Jump one line at line 8
                dbne    d7, .3
                cmp.w   #2, d7                  ; Jump one line at line 16
                dbne    d7, .3
                dbra    d7, .1                  ; Go to next line

                moveq   #18, d7                 ; Line count for second column
                move.w  #PLANE_A_DEF+64*2*2+40, d6  ; VRAM address of the beginning of the second column
                dbra    d4, .1                  ; Update second column


                lea     bda_pkt_header.w, a7
                jmp     bda_enter_panic

beh_vdp_regs
                db      VDPR00                  ; #00
                db      VDPR01 | VDPDISPEN      ; #01
                db      VDPR02 | (PLANE_A_DEF >> 10 & $38)      ; #02 - Plane A
                db      VDPR03                  ; #03 - Window
                db      VDPR04 | (PLANE_A_DEF >> 13)            ; #04 - Plane B (same as plane A)
                db      VDPR05                  ; #05 - Sprite attributes
                db      VDPR06                  ; #06
                db      VDPR07                  ; #07
                db      VDPR08                  ; #08
                db      VDPR09                  ; #09
                db      VDPR10                  ; #10
                db      VDPR11                  ; #11
                db      VDPR12 | VDPH40         ; #12
                db      VDPR13                  ; #13 - Horizontal scroll
                db      VDPR14                  ; #14
                db      VDPR15 | 2              ; #15 - autoincrement
                db      VDPR16 | VDPSCRH64      ; #16 - Plane size
                db      VDPR17                  ; #17
                db      VDPR18                  ; #18
                align   2

                if TARGET == TARGET_SCD1 || TARGET == TARGET_SCD2
beh_init_scd_vectors
                moveq   #(beh_scd_vectors_end - beh_scd_vectors)/6 - 1, d0  ; Loop count
                move.w  #$4EF9, d1      ; JMP instruction
                lea beh_scd_vectors, a0 ; Vector table
.vec_loop
                movea.w  (a0)+, a1      ; Read vector to alter
                move.l  (a1), a1        ; Go to vector target
                move.w  d1, (a1)+       ; Write JMP
                move.l  (a0)+, (a1)     ; Write JMP target address
                dbra    d0, .vec_loop
                rts

beh_scd_vectors
                dw  $0C
                dl  int_addrerr
                dw  $14
                dl  int_zdiv
                dw  $18
                dl  int_chk
                dw  $1C
                dl  int_trapv
                dw  $20
                dl  int_priv
                dw  $28
                dl  int_linea
                dw  $2C
                dl  int_linef
                dw  $80
                dl  int_trap00
beh_scd_vectors_end
                endif

beh_font
                hex     00000000 00000000
                hex     3C42464A 52623C00
                hex     10305010 10107C00
                hex     3C42020C 30427E00
                hex     3C42021C 02423C00
                hex     08182848 FE081C00
                hex     7E407C02 02423C00
                hex     1C20407C 42423C00
                hex     7E420408 10101000
                hex     3C42423C 42423C00
                hex     3C42423E 02043800
                hex     18244242 7E424200
                hex     7C22223C 22227C00
                hex     1C224040 40221C00
                hex     78242222 22247800
                hex     7E222838 28227E00
                hex     7E222838 28207000
beh_font_end

        endif   ; ..UNDEF BLS_NBEH

; vim: ts=8 sw=8 sts=8 et
