        if ..UNDEF BLS_VDP_INC
BLS_VDP_INC     set     1

; VDP address mappings - can be changed to use registers
VDPDATA         set     $C00000
VDPCTRL         set     $C00004
VDPHV           set     $C00008
VDPUSEADDR      macro   ; Use address constants
_VDPREG         set     0
                endm
VDPUSEREG       macro   ; Use a4 for VDPDATA and a5 for VDPCTRL
_VDPREG         set     1
                endm
VDPSETADDRREG   macro   ; Use a4 / a5 and set them
                VDPUSEREG
                lea     VDPDATA, a4
                lea     VDPCTRL, a5
                endm
                VDPUSEADDR

; Security code for genesis boot
VDPSECURITY     macro
        if TARGET == TARGET_GEN
                move.b  d0, -(a7)
                move.b  $A10001, d0
                andi.b  #$0F, d0
                beq.b   oldgen\?
                move.l  $100.w, $A14000
oldgen\?
                move.b  (a7)+, d0
        endif
                endm

VDPST_EMPTY set $0200   ; VDP FIFO empty
VDPST_FULL  set $0100   ; VDP FIFO full
VDPST_F     set $0080   ; V interrupt happened
VDPST_SOVR  set $0040   ; Sprite overflow
VDPST_C     set $0020   ; Sprite collision
VDPST_ODD   set $0010   ; Odd frame in interlace mode
VDPST_VB    set $0008   ; 1 during vblank, 0 otherwise
VDPST_HB    set $0004   ; 1 during hblank, 0 otherwise
VDPST_DMA   set $0002   ; 1 if DMA is busy
VDPST_PAL   set $0001   ; 1 if PAL display mode, 0 if NTSC

PLANE_A_DEF     set     $C000
PLANE_B_DEF     set     $E000
WINDOW_DEF      set     $B000
SPRAT_DEF       set     $A000
HSCROLL_TABLE_DEF set   $A800

VDPR00          set     $04             ; VDP register #00
VDPHINT         set     $10             ; Enable HBlank interrupt (level 4)
VDPHVEN         set     $02             ; Enable read HV counter

VDPR01          set     $04             ; VDP register #01
VDPDISPEN       set     $40             ; Display enable
VDPVINT         set     $20             ; Enable VBlank interrupt (level 6)
VDPDMAEN        set     $10             ; Enable DMA
VDPV30          set     $08             ; Display 30 cells vertically (PAL only)

VDPR02          set     $00             ; VDP register #02 - plane a name table
VDPR03          set     $00             ; VDP register #03 - window name table
VDPR04          set     $00             ; VDP register #04 - plane b name table
VDPR05          set     $00             ; VDP register #05 - sprite attrib table
VDPR06          set     $00             ; VDP register #06
VDPR07          set     $00             ; VDP register #07
VDPR08          set     $00             ; VDP register #08
VDPR09          set     $00             ; VDP register #09
VDPR10          set     $00             ; VDP register #10 - hblank period

VDPR11          set     $00             ; VDP register #11
VDPEINT         set     $08             ; Enable external interrupt (level 2)
VDPVSCRSCROLL   set     $00             ; V scroll : per screen
VDPVCELLSCROLL  set     $04             ; V scroll : 2 cells
VDPHSCRSCROLL   set     $00             ; H scroll : per screen
VDPHCELLSCROLL  set     $02             ; H scroll : per cell
VDPHLINESCROLL  set     $03             ; H scroll : per line

VDPR12          set     $00             ; VDP register #12
VDPH40          set     $81             ; Display 40 cells horizontally
VDPSTE          set     $40             ; Enable shadow/hilight
VDPILACE        set     $02             ; Interlace mode
VDPILACEDBL     set     $06             ; Interlace mode (double resolution)


VDPR13          set     $00             ; VDP register #13 - hscroll table
VDPR14          set     $00             ; VDP register #14
VDPR15          set     $00             ; VDP register #15 : autoincrement

VDPR16          set     $00             ; VDP register #16
VDPSCRV32       set     $00             ; VDP scroll 32 cells vertical
VDPSCRV64       set     $10             ; VDP scroll 64 cells vertical
VDPSCRV128      set     $30             ; VDP scroll 128 cells vertical
VDPSCRH32       set     $00             ; VDP scroll 32 cells horizontal
VDPSCRH64       set     $01             ; VDP scroll 64 cells horizontal
VDPSCRH128      set     $03             ; VDP scroll 128 cells horizontal

VDPR17          set     $00             ; VDP register #17
VDPWINRIGHT     set     $80             ; Window on the right hand side

VDPR18          set     $00             ; VDP register #18
VDPWINBOT       set     $80             ; Window at the bottom of the screen

                ; Upload a byte table to VDP registers 00-18
                ; Does not set DMA registers.
                ; reg_table = address of the register table
                ; Destroys d0-d1/a0
VDPSETALLREGS   macro   reg_table
                lea     reg_table, a0
        if ! _VDPREG
                lea     VDPCTRL, a1     ; Use a temporary register for faster access
        endif
                moveq   #18, d0
                move.w  #$8000, d1
vdpsetuploop\?  move.b  (a0)+, d1
        if _VDPREG
                move.w  d1, (a5)
        else
                move.w  d1, (a1)
        endif
                addi.w  #$100, d1
                dbra    d0, vdpsetuploop\?
                endm

VDPSETREG       macro   r, v            ; Set a VDP register r to value v
        if _VDPREG
                move.w  #($8000 | (r<<8) | v), (a5)
        else
                move.w  #($8000 | (r<<8) | v), VDPCTRL
        endif
                endm

VDPPREPREG      macro   r, v
                dw      #($8000 | (r<<8) | v)
                endm

VDPSETREG2      macro   r1, v1, r2, v2  ; Set twp VDP registesr r1 and r2 to values v1 and v2 respectively
        if _VDPREG
                move.l  #((($8000 | (r1<<8) | v1) << 16) | ($8000 | (r2<<8) | v2)), (a5)
        else
                move.l  #((($8000 | (r1<<8) | v1) << 16) | ($8000 | (r2<<8) | v2)), VDPCTRL
        endif
                endm

VDPSETAUTOINCR  macro   s               ; Set VDP auto increment by writing to register 15
                VDPSETREG 15, s
                endm

VDPPREPAUTOINCR macro   s
                VDPPREPREG 15, s
        endm

VDPENABLE       macro   display, hint, vint, dma
.R00\?          set     VDPR00 | VDPHVEN
.R01\?          set     VDPR01
        if display
.R01\?          set     .R01\? | VDPDISPEN
        endif
        if hint
.R00\?          set     .R00\? | VDPHINT
        endif
        if vint
.R01\?          set     .R01\? | VDPVINT
        endif
        if dma
.R01\?          set     .R01\? | VDPDMAEN
        endif
                VDPSETREG2 0, .R00\?, 1, .R01\?
                endm

VDPWAITLINE     macro   line
.waitline\?
                move.w  VDPHV, -(sp)            ; Read HV counter with a word access
                cmpi.b  #line, (sp)             ; Compare V part with the line
                addq.l  #2, sp                  ; Pop the value (CCR not affected)
                bne.b   .waitline\?             ; Wait if line number is different
                endm

VDPUNTILLINE    macro   line
.waitline\?
                move.w  VDPHV, -(sp)            ; Read HV counter with a word access
                cmpi.b  #line, (sp)             ; Compare V part with the line
                addq.l  #2, sp                  ; Pop the value (CCR not affected)
                blo.b   .waitline\?             ; Wait if line number is different
                endm

VRAM            equ     $00000000
CRAM            equ     $80000000
VSRAM           equ     $00000010

VDPSETREAD      macro   addr, ram       ; Setup VDP to read from address addr from specified ram
        if _VDPREG
                move.l  #(((addr & $3FFF) << 16) | (addr >> 14) | (ram & $7FFFFFFF) | ((ram >> 26) & $20)), (a5)
        else
                move.l  #(((addr & $3FFF) << 16) | (addr >> 14) | (ram & $7FFFFFFF) | ((ram >> 26) & $20)), VDPCTRL
        endif
                endm

VDPSETWRITE     macro   addr, ram       ; Setup VDP to write to address addr into specified ram
        if _VDPREG
                move.l  #(((addr & $3FFF) << 16) | (addr >> 14) | $40000000 | ram), (a5)
        else
                move.l  #(((addr & $3FFF) << 16) | (addr >> 14) | $40000000 | ram), VDPCTRL
        endif
                endm

                ; Read from data port to target
VDPREADW        macro   target
        if _VDPREG
                move.w  (a4), target
        else
                move.w  VDPDATA, target
        endif
                endm
VDPREADL        macro   target
        if _VDPREG
                move.l  (a4), target
        else
                move.l  VDPDATA, target
        endif
                endm

                ; Write value to data port
VDPWRITEW       macro   value
        if _VDPREG
                move.w  value, (a4)
        else
                move.w  value, VDPDATA
        endif
                endm
VDPWRITEL       macro   value
        if _VDPREG
                move.l  value, (a4)
        else
                move.l  value, VDPDATA
        endif
                endm

VDPSETREADVAR   macro   reg, ram        ; Setup VDP to read from address (reg.w) of specified ram. Alters d0.
                move.w  reg, d0         ; Store register
                ; Compute first half of the command
                andi.w  #$3FFF, d0
                ori.w   #(ram >> 16) & $7FFF, d0
                ; Send first half
        if _VDPREG
                move.w  d0, (a5)
        else
                move.w  d0, VDPCTRL
        endif
                move.w  reg, d0         ; Recall register
                ; Compute second half of the command
                rol.w   #2, d0          ; Place 2 MSB at the 2 LSB
                andi.w  #$0003, d0
                ori.w   #(ram | ((ram >> 26) & $20)) & $FFFF, d0
                ; Send second half
        if _VDPREG
                move.w  d0, (a5)
        else
                move.w  d0, VDPCTRL
        endif
                endm

VDPSETWRITEVAR  macro   reg, ram        ; Setup VDP to write to address (reg.w) of specified ram. Alters d0.
                move.w  reg, d0         ; Store register
                ; Compute first half of the command
                andi.w  #$3FFF, d0
                ori.w   #(ram >> 16) | $4000, d0
                ; Send first half
        if _VDPREG
                move.w  d0, (a5)
        else
                move.w  d0, VDPCTRL
        endif
                move.w  reg, d0         ; Recall register
                ; Compute second half of the command
                rol.w   #2, d0         ; Place 2 MSB at the 2 LSB
                andi.w  #$0003, d0
                ori.w   #ram & $FFFF, d0
                ; Send second half
        if _VDPREG
                move.w  d0, (a5)
        else
                move.w  d0, VDPCTRL
        endif
                endm

VDPSETDMASIZE   macro   s
                VDPSETREG2 19, (s & $FF), 20, ((s >> 8) & $FF)
                endm

VDPPREPDMASIZE  macro   s
                VDPPREPREG 19, (s & $FF)
                VDPPREPREG 20, ((s >> 8) & $FF)
                endm

VDPDMAWAIT  macro       ; Wait until last DMA operation is finished
                moveq   #VDPST_DMA, d1
.dmawait\?
        if _VDPREG
                move.w  (a5), d0
        else
                move.w  VDPCTRL, d0
        endif
                and.w   d1, d0
                bne.b   .dmawait\?
                endm

VDPFIFOWAIT     macro
                move.w  #VDPST_EMPTY, d1
.fifowait\?
        if _VDPREG
                move.w  (a5), d0
        else
                move.w  VDPCTRL, d0
        endif
                and.w   d1, d0
                beq.b   .fifowait\?
                endm

VDPWAIT         macro
                VDPDMAWAIT
                VDPFIFOWAIT
                endm

VDPSEND         macro   src, tgt, sz, ram       ; Copy without using DMA
                VDPSETWRITE tgt, ram
                lea     src, a0
        if sz / 4 <= 0x100
                moveq   #(sz / 4) - 1, d0
        else
                move.w  #(sz / 4) - 1, d0
        endif
.writeloop\?
        if _VDPREG
                move.l  (a0)+, (a4)
        else
                move.l  (a0)+, VDPDATA
        endif
                dbra    d0, .writeloop\?

        if sz & 2
        if _VDPREG
                move.w  (a0)+, (a4)
        else
                move.w  (a0)+, VDPDATA
        endif
        endif
                endm

VDPDMASEND      macro   src, tgt, sz, ram       ; Copy sz bytes to vram address tgt from address src
                                                ; DMA must be enabled beforehand and autoincrement must be set correctly

                VDPSETDMASIZE sz / 2    ; Set DMA size

        if (src & $F00000) == $200000
SOFF            set     2
        else
SOFF            set     0
        endif

                ; Set DMA source address and copy mode
                VDPSETREG2 21, (((src+soff) >> 1) & $FF), 22, (((src+soff) >> 9) & $FF)
                VDPSETREG 23, (((src+soff) >> 17) & $7F)

        if _VDPREG
                ; Set destination address and start DMA
                move.w  #((tgt & $3FFF) | (ram >> 16) | $4000), (a5)
                move.w  #((tgt >> 14) | (ram & $FFFF) | $80), -(a7)     ; Use stack to ensure RAM access during DMA
                move.w  (a7), (a5)                                      ; Write destination address with word write
                addq.l  #2, a7                                          ; (circumvents documented hardware flaws)

        else
                ; Set destination address and start DMA
                move.w  #((tgt & $3FFF) | (ram >> 16) | $4000), VDPCTRL
                move.w  #((tgt >> 14) | (ram & $FFFF) | $80), -(a7)     ; Use stack to ensure RAM access during DMA
                move.w  (a7), VDPCTRL                                   ; Write destination address with word write
                addq.l  #2, a7                                          ; (circumvents documented hardware flaws)

        endif
        if SOFF == 2
                ; Circumvent hardware flaw in WRAM DMA
                VDPSETWRITE tgt, ram
                VDPWRITEW src
        endif
                endm

VDPPREPDMASEND  macro   src, tgt, sz, ram
                VDPPREPDMASIZE sz / 2
                VDPPREPREG 21, ((src >> 1) & $FF)
                VDPPREPREG 22, ((src >> 9) & $FF)
                VDPPREPREG 23, ((src >> 17) & $7F)
                dw      ((tgt & $3FFF) | (ram >> 16) | $4000)
                dw      ((tgt >> 14) | (ram & $FFFF) | $80)
                endm

VDPDMAFILL      macro   data, tgt, sz   ; Fill sz bytes of vram at address tgt using data byte
                                                ; DMA must be enabled beforehand and autoincrement must be set correctl
                                                ; Auto increment must be set to 1
                assert sz >= 2
                assert tgt + sz <= $10000

                VDPSETDMASIZE (sz - 1)          ; Set DMA size

                ; Set DMA fill mode
                VDPSETREG 23, $80

        if _VDPREG
                ; Set destination address
                move.l  #(((tgt << 16) & $3FFF0000) | (tgt >> 14) | $40000080), (a5)
                ; Set data word and start DMA
                move.b  data, (a4)
        else
                ; Set destination address
                move.l  #(((tgt << 16) & $3FFF0000) | (tgt >> 14) | $40000080), VDPCTRL
                ; Set data word and start DMA
                move.b  data, VDPDATA
        endif

                endm
        

VDPSETPALETTE   macro   data, palette           ; Set a 16 color palette using data. palette is 0 to 3.
                VDPSETREG 15, 2
                VDPDMASEND data, (palette * 32), 32, CRAM
                endm

VDPPREPPALETTE  macro   data, palette
                VDPPREPREG 15, 2
                VDPPREPDMASEND data, (palette * 32), 32, CRAM
                endm

VDPSETBG        macro   color
                VDPSETWRITE 0, CRAM
        if _VDPREG
                move.w  color, (a4)
        else
                move.w  color, VDPDATA
        endif
                endm

VDPSETWRITEHSCROLL      macro   line            ; Place VDP write address in hscroll table
                VDPSETWRITE (line * 4 + HSCROLL_TABLE), VRAM
                endm

VDPSETWRITEVSCROLL      macro   column          ; Place VDP write address in vscroll table
                VDPSETWRITE (column * 4), VSRAM
                endm

VDPSETWRITEPLANE        macro   plane, w, x, y  ; Place VDP write address in plane name table for a screen width of W tiles
                VDPSETWRITE (x * 2 + y * w * 2 + plane), VRAM
                endm

VDPPREPSTART    macro
                dw      ((.__vdpprepterminate-.__vdpprepstart) / 2) - 1
.__vdpprepstart
                endm

VDPPREPTERMINATE        macro
.__vdpprepterminate
                endm

VDPTILE_PRIO    equ     $8000

VDPTILE_PAL0    equ     $6000
VDPTILE_PAL1    equ     $2000
VDPTILE_PAL2    equ     $4000
VDPTILE_PAL3    equ     $6000

VDPTILE_VMIR    equ     $1000
VDPTILE_HMIR    equ     $0800


        endif

; vim: ts=8 sw=8 sts=8 et
