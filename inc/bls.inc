; Main include for Blast ! SDK

; Bus constants
BUS_NONE	equ	-1
BUS_MAIN	equ	0
BUS_SUB		equ	1
BUS_Z80		equ	2

; Chip constants
CHIP_NONE	equ	-1
CHIP_CART	equ	0
CHIP_BRAM	equ	1
CHIP_ZRAM	equ	2
CHIP_VRAM	equ	3
CHIP_RAM	equ	4
CHIP_PRAM	equ	5
CHIP_WRAM	equ	6
CHIP_PCM	equ	7

rti		macro
		if SCD == 0
		rte
		else
		rts
		endif
		endm

; Controllers
; Mapping :
; Bit 7  6  5  4  3  2  1  0
;       TH TR TL RT LT DN UP
; 	 0  C  B RT LT DN UP
;        1 ST  A  -  - DN UP

; Game controller lines
CUP		equ	$01
BCUP		equ	0
CDOWN		equ	$02
BCDOWN		equ	1
CLEFT		equ	$04
BCLEFT		equ	2
CRIGHT		equ	$08
BCRIGHT		equ	3
CTL		equ	$10
BCTL		equ	4
CTR		equ	$20
BCTR		equ	5
CTH		equ	$40
BCTH		equ	6
CSEL		equ	CTH
BCSEL		equ	BCTH
CBTNB		equ	CTL
BCBTNB		equ	BCTL
CBTNA		equ	CTL
BCBTNA		equ	BCTL
CBTNC		equ	CTR
BCBTNC		equ	BCTR
CBTNSTART	equ	CTR
BCBTNSTART	equ	BCTR

; CCTRL bits
CTHINT		equ	$80 ; Enable TH interrupt

; Game controller registers
CDATA1		equ	$A10003
CDATA2		equ	$A10005
CDATA3		equ	$A10007

CCTRL1		equ	$A10009
CCTRL2		equ	$A1000B
CCTRL3		equ	$A1000D

STX1		equ	$A1000F
SRX1		equ	$A10011
SCTRL1		equ	$A10013
STX2		equ	$A10015
SRX2		equ	$A10017
SCTRL2		equ	$A10019
STX3		equ	$A1001B
SRX3		equ	$A1001D
SCTRL3		equ	$A1001F

; SCTRL bits
SCTFULL		equ	$01
SCRRDY		equ	$02
SCRERR		equ	$04
SCRINT		equ	$08
SCSOUT		equ	$10
SCSIN		equ	$20
SC300		equ	$C0
SC1200		equ	$80
SC2400		equ	$40
SC4800		equ	$00

; Z80
ZBUSREQ		equ	$A11100
ZRESET		equ	$A11200
ZREQ		equ	$0100			; Value to be written to ZBUSREQ or ZRESET

		include	bls_vdp.inc

; Sound defines

		if BUS == BUS_Z80
FM1REG		equ	$4000
FM1DATA		equ	$4001
FM2REG		equ	$4002
FM2DATA		equ	$4003
PSG		equ	$7F11
		else
FM1REG		equ	$A04000
FM1DATA		equ	$A04001
FM2REG		equ	$A04002
FM2DATA		equ	$A04003
PSG		equ	$C00011
		endif

		include	bls_macros.inc

		if SCD != 0
		include	blsscd.inc
		endif
