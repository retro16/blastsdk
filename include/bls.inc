; Main include for Blast ! SDK
; It is automatically included by blsgen

            if ..UNDEF BLS_VERSION
BLS_VERSION set 1

; Bus constants
BUS_NONE        equ     0
BUS_MAIN        equ     1
BUS_SUB         equ     2
BUS_Z80         equ     3

; Chip constants
CHIP_NONE       equ     0
CHIP_MSTACK equ 1
CHIP_SSTACK equ 2
CHIP_ZSTACK equ 3
CHIP_CART       equ     4
CHIP_BRAM       equ     5
CHIP_ZRAM       equ     6
CHIP_VRAM       equ     7
CHIP_RAM        equ     8
CHIP_PRAM       equ     9
CHIP_WRAM       equ     10
CHIP_PCM        equ     11

; Target constants
TARGET_UNKNOWN  equ 0
TARGET_GEN      equ 1
TARGET_SCD1     equ 2
TARGET_SCD2     equ 3
TARGET_RAM      equ 4

                if BUS == BUS_MAIN
; Controllers
; Mapping :
; Bit 7  6  5  4  3  2  1  0
;       TH TR TL RT LT DN UP
;        1  C  B RT LT DN UP
;        0 ST  A  -  - DN UP

; Game controller lines
CUP             equ     $01
CUP_BIT         equ     0
CDOWN           equ     $02
CDOWN_BIT               equ     1
CLEFT           equ     $04
CLEFT_BIT               equ     2
CRIGHT          equ     $08
CRIGHT_BIT              equ     3
CTL             equ     $10
CTL_BIT         equ     4
CTR             equ     $20
CTR_BIT         equ     5
CTH             equ     $40
CTH_BIT         equ     6
CSEL            equ     CTH
CSEL_BIT                equ     CTH_BIT
CSEL_ASTART     equ     0
CSEL_LRBC       equ     CSEL
CBTNB           equ     CTL
CBTNB_BIT               equ     CTL_BIT
CBTNA           equ     CTL
CBTNA_BIT               equ     CTL_BIT
CBTNC           equ     CTR
CBTNC_BIT               equ     CTR_BIT
CBTNSTART       equ     CTR
CBTNSTART_BIT   equ     CTR_BIT

; CCTRL bits
CTHINT          equ     $80 ; Enable TH interrupt

; Game controller registers
CDATA1          equ     $A10003
CDATA2          equ     $A10005
CDATA3          equ     $A10007

CCTRL1          equ     $A10009
CCTRL2          equ     $A1000B
CCTRL3          equ     $A1000D

                ; Debug protocol control and data ports
                ; To use the "EXT" port of the old genesis, specify CCTRL3 / CDATA3
BDACTRL         equ     CCTRL2
BDADATA         equ     CDATA2
BDPCTRL         equ     BDACTRL
BDPDATA         equ     BDADATA

STX1            equ     $A1000F
SRX1            equ     $A10011
SCTRL1          equ     $A10013
STX2            equ     $A10015
SRX2            equ     $A10017
SCTRL2          equ     $A10019
STX3            equ     $A1001B
SRX3            equ     $A1001D
SCTRL3          equ     $A1001F

; SCTRL bits
SCTFULL         equ     $01
SCRRDY          equ     $02
SCRERR          equ     $04
SCRINT          equ     $08
SCSOUT          equ     $10
SCSIN           equ     $20
SC300           equ     $C0
SC1200          equ     $80
SC2400          equ     $40
SC4800          equ     $00

; Z80
ZBUSREQ         equ     $A11100
ZRESET          equ     $A11200
ZREQ            equ     $0100                   ; Value to be written to ZBUSREQ or ZRESET

; Sound defines
                if BUS == BUS_Z80
FM1REG          equ     $4000
FM1DATA         equ     $4001
FM2REG          equ     $4002
FM2DATA         equ     $4003
PSG             equ     $7F11
                else
FM1REG          equ     $A04000
FM1DATA         equ     $A04001
FM2REG          equ     $A04002
FM2DATA         equ     $A04003
PSG             equ     $C00011
                endif
                endif
                
; Interrupt vector offsets
INTV_BUSERR     set     $08
INTV_ADDRERR    set     $0C
INTV_ILL        set     $10
INTV_ZDIV       set     $14
INTV_CHK        set     $18
INTV_TRAPV      set     $1C
INTV_PRIV       set     $20
INTV_TRACE      set     $24
INTV_LINEA      set     $28
INTV_LINEF      set     $2C

INTV_SPURIOUS   set     $60
INTV_LEVEL1     set     $64
INTV_LEVEL2     set     $68
INTV_PAD        set     INTV_LEVEL2
INTV_LEVEL3     set     $6C
INTV_LEVEL4     set     $70
INTV_HBLANK     set     INTV_LEVEL4
INTV_LEVEL5     set     $74
INTV_LEVEL6     set     $78
INTV_VBLANK     set     INTV_LEVEL6
INTV_LEVEL7     set     $7C

INTV_TRAP00     set     $80
INTV_TRAP01     set     $84
INTV_TRAP02     set     $88
INTV_TRAP03     set     $8C
INTV_TRAP04     set     $90
INTV_TRAP05     set     $94
INTV_TRAP06     set     $98
INTV_TRAP07     set     $9C
INTV_TRAP08     set     $A0
INTV_TRAP09     set     $A4
INTV_TRAP10     set     $A8
INTV_TRAP11     set     $AC
INTV_TRAP12     set     $B0
INTV_TRAP13     set     $B4
INTV_TRAP14     set     $B8
INTV_TRAP15     set     $BC
                
        if BUS == BUS_MAIN
                include bls_vdp.inc
        endif

                include bls_macros.inc
                include bls_routines.inc

        if TARGET == TARGET_SCD1 || TARGET == TARGET_SCD2
                include blsscd.inc
        endif

        endif

; vim: ts=8 sw=8 sts=8 et
