		if BUS == BUS_MAIN
		; Genesis interrupt vector table
G_INTV_ADDRERR	set	$FFFD80	; 0x0C
G_INTV_ILL	set	$FFFD80	; 0x10
G_INTV_ZDIV	set	$FFFD86	; 0x14
G_INTV_CHK	set	$FFFD7A	; 0x18
G_INTV_TRAPV	set	$FFFD8C	; 0x1C
G_INTV_PRIV	set	$FFFD9E	; 0x20
G_INTV_TRACE	set	$FFFDA4	; 0x24
G_INTV_LINEA	set	$FFFD92	; 0x28
G_INTV_LINEF	set	$FFFD98	; 0x2C

G_INTV_LEVEL2	set	$FFFD14	; 0x68
G_INTV_PAD	set	G_INTV_LEVEL2
G_INTV_LEVEL4	set	$A12006	; 0x70
G_INTV_HBLANK	set	G_INTV_LEVEL4
G_INTV_LEVEL6	set	$FFFD08	; 0x78
G_INTV_VBLANK	set	G_INTV_LEVEL6

G_INTV_TRAP00	set	$FFFD1A	; 0x80
G_INTV_TRAP01	set	$FFFD20
G_INTV_TRAP02	set	$FFFD26
G_INTV_TRAP03	set	$FFFD2C
G_INTV_TRAP04	set	$FFFD32
G_INTV_TRAP05	set	$FFFD38
G_INTV_TRAP06	set	$FFFD3E
G_INTV_TRAP07	set	$FFFD44
G_INTV_TRAP08	set	$FFFD4A
G_INTV_TRAP09	set	$FFFD50
G_INTV_TRAP10	set	$FFFD56
G_INTV_TRAP11	set	$FFFD5C
G_INTV_TRAP12	set	$FFFD62
G_INTV_TRAP13	set	$FFFD68
G_INTV_TRAP14	set	$FFFD6E
G_INTV_TRAP15	set	$FFFD74	; 0xBC

GATE_ARRAY	set	$A12000

		else
G_INTV_BUSERR	set	$08
G_INTV_ADDRERR	set	$0C
G_INTV_ILL	set	$10
G_INTV_ZDIV	set	$14
G_INTV_CHK	set	$18
G_INTV_TRAPV	set	$1C
G_INTV_PRIV	set	$20
G_INTV_TRACE	set	$24
G_INTV_LINEA	set	$28
G_INTV_LINEF	set	$2C

G_INTV_SPURIOUS	set	$60
G_INTV_LEVEL1	set	$64
G_INTV_LEVEL2	set	$68
G_INTV_LEVEL3	set	$6C
G_INTV_LEVEL4	set	$70
G_INTV_LEVEL5	set	$74
G_INTV_LEVEL6	set	$78
G_INTV_LEVEL7	set	$7C

G_INTV_TRAP00	set	$80
G_INTV_TRAP01	set	$84
G_INTV_TRAP02	set	$88
G_INTV_TRAP03	set	$8C
G_INTV_TRAP04	set	$90
G_INTV_TRAP05	set	$94
G_INTV_TRAP06	set	$98
G_INTV_TRAP07	set	$9C
G_INTV_TRAP08	set	$A0
G_INTV_TRAP09	set	$A4
G_INTV_TRAP10	set	$A8
G_INTV_TRAP11	set	$AC
G_INTV_TRAP12	set	$B0
G_INTV_TRAP13	set	$B4
G_INTV_TRAP14	set	$B8
G_INTV_TRAP15	set	$BC

GATE_ARRAY	set	$FF8000
SUBCODE		set	$FF8100	; Subcode buffer (size = $80)
		endif


; Gate array defines

GA_RH		set	GATE_ARRAY + $00	; Reset and halt
GA_NORESET	set	$0001
GA_BUSREQ	set	$0002
GA_VERSION	set	$00F0
		if BUS == BUS_MAIN
GA_IFL2		set	$0100
GA_IEN2		set	$8000
		else
GA_LEDG		set	$0100
GA_LEDR		set	$0200
		endif

GA_MM		set	GATE_ARRAY + $02	; Memory mode
GA_WP0		set	$0100			; Write protect
GA_WP1		set	$0200
GA_WP2		set	$0400
GA_WP3		set	$0800
GA_WP4		set	$1000
GA_WP5		set	$2000
GA_WP6		set	$4000
GA_WP7		set	$8000
		if BUS == BUS_MAIN
GA_BK0		set	$0040
GA_BK1		set	$0080
		else
GA_PM0		set	$0008
GA_PM1		set	$0010
GA_PM_WDOWN	set	GA_PM0
GA_PM_WUP	set	GA_PM1
		endif
GA_MODE		set	$0004
GA_DMNA		set	$0002
GA_RET		set	$0001

GA_CDC		set	GATE_ARRAY + $04	; CDC
GA_DD0		set	$0100
GA_DD1		set	$0200
GA_DD2		set	$0400
GA_DD_MAINREAD	set	$0200
GA_DD_SUBREAD	set	$0300
GA_DD_PCMDMA	set	$0400
GA_DD_WORDDMA	set	$0700
GA_EDT		set	$8000
GA_DSR		set	$4000
GA_CDCRS1	set	GATE_ARRAY + $06	; CDC register data
GA_CDCHD	set	GATE_ARRAY + $08	; CDC host data
GA_CDCDMA	set	GATE_ARRAY + $0A	; Target address for CDC DMA
GA_STOPWATCH	set	GATE_ARRAY + $0C	; Stopwatch (30.72 us)

GA_COMMFLAGS	set	GATE_ARRAY + $0E	; MSB writable by main, LSB writable by sub
GA_COMMFLAGS_MAIN	set	GATE_ARRAY + $0E	; Byte access
GA_COMMFLAGS_SUB 	set	GATE_ARRAY + $0F	; Byte access
GA_COMMWORDS	set	GATE_ARRAY + $10
		if BUS == BUS_MAIN
GA_COMMOUT	set	GA_COMMWORDS		; Read-write
GA_COMMIN	set	GA_COMMWORDS + $10	; Read-only
		else
GA_COMMOUT	set	GA_COMMWORDS + $10	; Read-write
GA_COMMIN	set	GA_COMMWORDS		; Read-only
		endif

GA_IMASK	set	GATE_ARRAY + $32	; Interrupt mask
GA_IM_L1	set	$0002			; Level 1 (graphic complete)
GA_IM_L2	set	$0004			; Level 2 (software int)
GA_IM_L3	set	$0008			; Level 3 (Timer reached 0)
GA_IM_L4	set	$0010			; Level 4 (CDD)
GA_IM_L5	set	$0020			; Level 5 (CDC error correction)
GA_IM_L6	set	$0040			; Level 6 (Subcode)

GA_FADER	set	GATE_ARRAY + $34
GA_FADER_MASK	set	$7FF0
GA_FADER_SSF	set	$0002			; Spindle speed flag
GA_FADER_DEF	set	$000C			; De-emphasis
GA_FADER_EFDT	set	$8000			; End of fade

GA_CDD		set	GATE_ARRAY + $36
GA_CDD_DTS	set	$0001			; Data transfer status
GA_CDD_DRS	set	$0002			; Data receive status
GA_CDD_HOCK	set	$0004			; Host clock
GA_CDD_DM	set	$0100			; Data/Muting (1 = mute)

GA_CDDCOM	set	GATE_ARRAY + $38	; CDD communication buffers

GA_FONTCOLOR	set	GATE_ARRAY + $4C
GA_FONTCOLOR0	set	$000F			; Color set for bit=0
GA_FONTCOLOR1	set	$00F0			; Color set for bit=1
GA_FONTBIT	set	GATE_ARRAY + $4E	; 2 bytes expanded into FONTDATA using FONTCOLOR
GA_FONTDATA	set	GATE_ARRAY + $50	; 8 bytes : expanded data (4 bytes per bit)

GA_STAMPSIZE	set	GATE_ARRAY + $58
GA_STS		set	$0002			; Set : 32x32, Clear : 16x16
GA_SMS		set	$0004			; Set : 16x16, Clear : 1x1
GA_RPT		set	$0001			; Repeat image if set

GA_STMAP	set	GATE_ARRAY + $5A	; Stamp map base address
GA_VCELLS	set	GATE_ARRAY + $5C	; Image V cell count minus one
GA_IMGBUF	set	GATE_ARRAY + $5E	; Image buffer address
GA_IMGOFFSET	set	GATE_ARRAY + $60	; Image buffer offset
GA_IMGHSIZE	set	GATE_ARRAY + $62	; Image width in pixels
GA_IMGVSIZE	set	GATE_ARRAY + $64	; Image height in pixels
GA_TVADDR	set	GATE_ARRAY + $66	; Trace vector address

GA_SCADDR	set	GATE_ARRAY + $68	; Subcode addddress

		include	blsscd_macros.inc
