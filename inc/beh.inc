                include bls.inc

        if SCD == 0
int_buserr      move.w  #$0008, -(a7)
                bra.w   beh_display_exception
                endif
int_addrerr     move.w  #$000C, -(a7)
                if SCD == 0
                bra.w   beh_display_exception
                else
                bra.b   beh_display_exception
                endif
                if SCD == 0
int_ill         move.w  #$0010, -(a7)
                bra.w   beh_display_exception
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
                if SCD == 0
int_spurious    move.w  #$0060, -(a7)
                bra.b   beh_display_exception
                endif
int_trap00      move.w  #$0080, -(a7)
                bra.b   beh_display_exception
int_trap01      move.w  #$0084, -(a7)
                bra.b   beh_display_exception
int_trap02      move.w  #$0088, -(a7)
                bra.b   beh_display_exception
int_trap03      move.w  #$008C, -(a7)
                bra.b   beh_display_exception
int_trap04      move.w  #$0090, -(a7)
                bra.b   beh_display_exception
int_trap05      move.w  #$0094, -(a7)
                bra.b   beh_display_exception
int_trap06      move.w  #$0098, -(a7)
                bra.b   beh_display_exception
int_trap08      move.w  #$00A0, -(a7)
                bra.b   beh_display_exception
int_trap09      move.w  #$00A4, -(a7)
                bra.b   beh_display_exception
int_trap10      move.w  #$00A8, -(a7)
                bra.b   beh_display_exception
int_trap11      move.w  #$00AC, -(a7)
                bra.b   beh_display_exception
int_trap12      move.w  #$00B0, -(a7)
                bra.b   beh_display_exception
int_trap13      move.w  #$00B4, -(a7)
                bra.b   beh_display_exception
int_trap14      move.w  #$00B8, -(a7)
                bra.b   beh_display_exception
int_trap15      move.w  #$00BC, -(a7)

; Displays the stack, one element per line
; Contains in order D0-D7, A0-A7, vector, exception stack frame, top of program stack
beh_display_exception
                move.w  #$2700, sr              ; Disable exceptions
                movem.l d0-d7/a0-a7, -(a7)      ; Push registers to be displayed

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

                moveq   #1, d4                  ; Column count
                moveq   #16, d7                 ; Line count
                move.w  #PLANE_A+64*2*2+12, d6  ; VRAM address of the beginning of the 3rd line

.1              VDPSETWRITEVAR d6, VRAM
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
                cmp.w   #9, d7                  ; Jump one line at line 8
                dbne    d7, .3
                dbra    d7, .1

                moveq   #16, d7                 ; Line count
                move.w  #PLANE_A+64*2*2+40, d6  ; VRAM address of the beginning of the second column
                dbra    d4, .1                  ; Update second column

beh_freeze      bra.b   beh_freeze

beh_vdp_regs
                db      VDPR00                  ; #00
                db      VDPR01 | VDPDISPEN      ; #01
                db      VDPR02                  ; #02 - Plane A
                db      VDPR03                  ; #03 - Window
                db      VDPR04                  ; #04 - Plane B (same as plane A)
                db      VDPR05                  ; #05 - Sprite attributes
                db      VDPR06                  ; #06
                db      VDPR07                  ; #07
                db      VDPR08                  ; #08
                db      VDPR09                  ; #09
                db      VDPR10                  ; #10
                db      VDPR11                  ; #11
                db      VDPR12 | VDPH40         ; #12
                db      VDPR13                  ; #13
                db      VDPR14                  ; #14
                db      VDPR15 | 2              ; #15 - autoincrement
                db      VDPR16 | VDPSCRH64      ; #16 - Plane size
                db      VDPR17                  ; #17
                db      VDPR18                  ; #18
        align   2

beh_init macro
        if SCD != 0
        jsr beh_init_scd_vectors
        endif
        endm

        if SCD != 0
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
        dw  $84
        dl  int_trap01
        dw  $88
        dl  int_trap02
        dw  $8C
        dl  int_trap03
        dw  $90
        dl  int_trap04
        dw  $94
        dl  int_trap05
        dw  $98
        dl  int_trap06
        dw  $A0
        dl  int_trap08
        dw  $A4
        dl  int_trap09
        dw  $A8
        dl  int_trap10
        dw  $AC
        dl  int_trap11
        dw  $B0
        dl  int_trap12
        dw  $B4
        dl  int_trap13
        dw  $B8
        dl  int_trap14
        dw  $BC
        dl  int_trap15
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

; vim: ts=8 sw=8 sts=8 et
