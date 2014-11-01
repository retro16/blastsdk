                if BUS == BUS_MAIN
GATE_ARRAY      set     $A12000
                else
SUB_SCRATCH     set     $000001D0               ; BDA scratch pad

GATE_ARRAY      set     $FFFF8000
SUBCODE         set     $FFFF8100               ; Subcode buffer (size = $80)
                endif


; Gate array defines

GA_RH           set     GATE_ARRAY + $00        ; Reset and halt
GA_NORESET      set     $0001
GA_NORESET_BIT  set     0
GA_BUSREQ       set     $0002
GA_BUSREQ_BIT   set     1
GA_VERSION      set     $00F0
                if BUS == BUS_MAIN
GA_IFL2         set     $0100
GA_IFL2_BIT     set     8
GA_IEN2         set     $8000
GA_IEN2_BIT     set     15
                else
GA_LEDG         set     $0100
GA_LEDG_BIT     set     8
GA_LEDR         set     $0200
GA_LEDR_BIT     set     9
                endif

GA_MM           set     GATE_ARRAY + $02        ; Memory mode
GA_WP0          set     $0100                   ; Write protect
GA_WP1          set     $0200
GA_WP2          set     $0400
GA_WP3          set     $0800
GA_WP4          set     $1000
GA_WP5          set     $2000
GA_WP6          set     $4000
GA_WP7          set     $8000
                if BUS == BUS_MAIN
GA_BK0          set     $0040
GA_BK1          set     $0080
                else
GA_PM0          set     $0008
GA_PM1          set     $0010
GA_PM_WDOWN     set     GA_PM0
GA_PM_WUP       set     GA_PM1
                endif
GA_MODE         set     $0004
GA_MODE_BIT     set     2
GA_DMNA         set     $0002
GA_DMNA_BIT     set     1
GA_RET          set     $0001
GA_RET_BIT      set     0

GA_CDC          set     GATE_ARRAY + $04        ; CDC
GA_DD0          set     $0100
GA_DD1          set     $0200
GA_DD2          set     $0400
GA_CDC_DEST     set     GA_CDC                  ; CDC data destination
GA_CDC_DEST_MAINREAD    set     $02
GA_CDC_DEST_SUBREAD     set     $03
GA_CDC_DEST_PCMDMA      set     $04
GA_CDC_DEST_PRAMDMA     set     $05
GA_CDC_DEST_WRAMDMA     set     $07
GA_CDC_DEST_DMA         set     $04             ; DMA flag
GA_CDC_DEST_DMA_BIT     set     2               ; DMA flag bit
GA_EDT          set     $8000                   ; End Data Transfert
GA_EDT_BIT      set     15
GA_DSR          set     $4000                   ; Data Set Ready
GA_DSR_BIT      set     14
                if BUS == BUS_MAIN
GA_HINT         set     GATE_ARRAY + $06        ; H-INT address
                else
GA_CDCRS1       set     GATE_ARRAY + $06        ; CDC register data
                endif
GA_CDCHD        set     GATE_ARRAY + $08        ; CDC host data
GA_CDCDMA       set     GATE_ARRAY + $0A        ; Target address for CDC DMA
GA_STOPWATCH    set     GATE_ARRAY + $0C        ; Stopwatch (30.72 us)

GA_COMMFLAGS    set     GATE_ARRAY + $0E        ; MSB writable by main, LSB writable by sub
GA_COMMFLAGS_MAIN       set     GATE_ARRAY + $0E        ; Byte access
GA_COMMFLAGS_SUB        set     GATE_ARRAY + $0F        ; Byte access
GA_COMMWORDS    set     GATE_ARRAY + $10
GA_COMMWORDS_MAIN set   GATE_ARRAY + $10
GA_COMMWORDS_SUB set    GATE_ARRAY + $20
                if BUS == BUS_MAIN
GA_COMMOUT      set     GA_COMMWORDS            ; Read-write
GA_COMMIN       set     GA_COMMWORDS + $10      ; Read-only
GA_COMMFLAGS_OUT        set     GATE_ARRAY + $0E        ; Byte access
GA_COMMFLAGS_IN         set     GATE_ARRAY + $0F        ; Byte access
                else
GA_COMMOUT      set     GA_COMMWORDS + $10      ; Read-write
GA_COMMIN       set     GA_COMMWORDS            ; Read-only
GA_COMMFLAGS_IN set     GATE_ARRAY + $0E        ; Byte access
GA_COMMFLAGS_OUT        set     GATE_ARRAY + $0F        ; Byte access
                endif

GA_IMASK        set     GATE_ARRAY + $32        ; Interrupt mask
GA_IM_L1        set     $0002                   ; Level 1 (graphic complete)
GA_IM_L2        set     $0004                   ; Level 2 (software int)
GA_IM_L3        set     $0008                   ; Level 3 (Timer reached 0)
GA_IM_L4        set     $0010                   ; Level 4 (CDD)
GA_IM_L5        set     $0020                   ; Level 5 (CDC error correction)
GA_IM_L6        set     $0040                   ; Level 6 (Subcode)
GA_IM_L1_BIT    set     1
GA_IM_L2_BIT    set     2
GA_IM_L3_BIT    set     3
GA_IM_L4_BIT    set     4
GA_IM_L5_BIT    set     5
GA_IM_L6_BIT    set     6

GA_FADER        set     GATE_ARRAY + $34
GA_FADER_MASK   set     $7FF0
GA_FADER_SSF    set     $0002                   ; Spindle speed flag
GA_FADER_DEF    set     $000C                   ; De-emphasis
GA_FADER_EFDT   set     $8000                   ; End of fade

GA_CDD          set     GATE_ARRAY + $36
GA_CDD_DTS      set     $0001                   ; Data transfer status
GA_CDD_DRS      set     $0002                   ; Data receive status
GA_CDD_HOCK     set     $0004                   ; Host clock
GA_CDD_DM       set     $0100                   ; Data/Muting (1 = mute)

GA_CDDCOM       set     GATE_ARRAY + $38        ; CDD communication buffers

GA_FONTCOLOR    set     GATE_ARRAY + $4C
GA_FONTCOLOR0   set     $000F                   ; Color set for bit=0
GA_FONTCOLOR1   set     $00F0                   ; Color set for bit=1
GA_FONTBIT      set     GATE_ARRAY + $4E        ; 2 bytes expanded into FONTDATA using FONTCOLOR
GA_FONTDATA     set     GATE_ARRAY + $50        ; 8 bytes : expanded data (4 bytes per bit)

GA_STAMPSIZE    set     GATE_ARRAY + $58
GA_STS          set     $0002                   ; Set : 32x32, Clear : 16x16
GA_STS_BIT      set     1
GA_SMS          set     $0004                   ; Set : 16x16, Clear : 1x1
GA_SMS_BIT      set     2
GA_RPT          set     $0001                   ; Repeat image if set
GA_RPT_BIT      set     0

GA_STMAP        set     GATE_ARRAY + $5A        ; Stamp map base address
GA_VCELLS       set     GATE_ARRAY + $5C        ; Image V cell count minus one
GA_IMGBUF       set     GATE_ARRAY + $5E        ; Image buffer address
GA_IMGOFFSET    set     GATE_ARRAY + $60        ; Image buffer offset
GA_IMGHSIZE     set     GATE_ARRAY + $62        ; Image width in pixels
GA_IMGVSIZE     set     GATE_ARRAY + $64        ; Image height in pixels
GA_TVADDR       set     GATE_ARRAY + $66        ; Trace vector address

GA_SCADDR       set     GATE_ARRAY + $68        ; Subcode address

                include blsscd_macros.inc

; vim: ts=8 sw=8 sts=8 et
