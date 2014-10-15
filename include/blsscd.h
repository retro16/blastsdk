#if BUS == BUS_MAIN
#define GATE_ARRAY 0xA12000
#else
#define SUB_SCRATCH ((volatile u8 *)0x00001D0)
#define GATE_ARRAY 0xFFFF8000
#define SUBCODE ((volatile u16 *)GATE_ARRAY + 0x100)
#endif


#define GA_RH ((volatile u16 *)GATE_ARRAY + 0x00) // Reset
#define GA_RH_BH ((volatile u8 *)GATE_ARRAY + 0x00)
#define GA_RH_BL ((volatile u8 *)GATE_ARRAY + 0x01)
#define GA_NORESET 0x0001
#define GA_NORESET_BL 0x01
#define GA_NORESET_BIT 0
#define GA_BUSREQ 0x0002
#define GA_BUSREQ_BL 0x02
#define GA_BUSREQ_BIT 1
#define GA_VERSION 0x00F0
#if BUS == BUS_MAIN
#define GA_IFL2 0x0100
#define GA_IFL2_BH 0x01
#define GA_IFL2_BIT 8
#define GA_IEN2 0x8000
#define GA_IEN2_BH 0x0x80
#define GA_IEN2_BIT 15
#else
#define GA_LEDG 0x0100
#define GA_LEDG_BH 0x01
#define GA_LEDG_BIT 8
#define GA_LEDR 0x0200
#define GA_LEDR_BH 0x02
#define GA_LEDR_BIT 9
#endif


#define GA_MM ((volatile u8 *)GATE_ARRAY + 0x03) // Memory mode
#define GA_WP ((volatile u8 *)GATE_ARRAY + 0x02) // Write protect
#if BUS = BUS_MAIN
#define GA_BK0 0x40
#define GA_BK1 0x80
#else
#define GA_PM0 0x08
#define GA_PM1 0x10
#endif
#define GA_MODE 0x04
#define GA_MODE_BIT 2
#define GA_DMNA 0x02
#define GA_DMNA_BIT 1
#define GA_RET 0x01
#define GA_RET_BIT 0


#define GA_CDC ((volatile u16 *)GATE_ARRAY + 0x04)
#define GA_CDC_BH ((volatile u8 *)GATE_ARRAY + 0x04)
#define GA_CDC_BL ((volatile u8 *)GATE_ARRAY + 0x05)
#define GA_DD0 0x0100
#define GA_DD1 0x0200
#define GA_DD2 0x0400
#define GA_CDC_DEST GA_CDC_BH
#define GA_CDC_DEST_MAINREAD 0x02
#define GA_CDC_DEST_SUBREAD 0x03
#define GA_CDC_DEST_PCMDMA 0x04
#define GA_CDC_DEST_PRAMDMA 0x05
#define GA_CDC_DEST_WRAMDMA 0x07
#define GA_CDC_DEST_DMA 0x04
#define GA_CDC_DEST_DMA_BIT 2
#define GA_EDT 0x8000
#define GA_EDT_BH 0x80
#define GA_EDT_BIT 15
#define GA_DSR 0x4000
#define GA_DSR_BH 0x40
#define GA_DSR_BIT 14


#if BUS == BUS_MAIN
#define GA_HINT ((volatile u16 *)GATE_ARRAY + 0x06)
#else
#define GA_CDCRS1 ((volatile u16 *)GATE_ARRAY + 0x06)
#endif

#define GA_CDCHD ((volatile u16 *)GATE_ARRAY + 0x08) // CDC Host Data

#define GA_CDCDMA ((volatile u16 *)GATE_ARRAY + 0x0A) // Target address of DMA

#define GA_STOPWATCH ((volatile u16 *)GATE_ARRAY + 0x0C)

#define GA_COMMFLAGS ((volatile u16 *)GATE_ARRAY + 0x0E)
#define GA_COMMFLAGS_MAIN ((volatile u8 *)GATE_ARRAY + 0x0E)
#define GA_COMMFLAGS_SUB ((volatile u8 *)GATE_ARRAY + 0x0F)
#define GA_COMMWORDS_B ((volatile u8 *)GATE_ARRAY + 0x10)
#define GA_COMMWORDS ((volatile u16 *)GATE_ARRAY + 0x10)
#define GA_COMMWORDS_L ((volatile u32 *)GATE_ARRAY + 0x10)
#define GA_COMMWORDS_MAIN_B ((volatile u8 *)GATE_ARRAY + 0x10)
#define GA_COMMWORDS_MAIN ((volatile u16 *)GATE_ARRAY + 0x10)
#define GA_COMMWORDS_MAIN_L ((volatile u32 *)GATE_ARRAY + 0x10)
#define GA_COMMWORD_MAIN_B(c) ((volatile u8 *)GATE_ARRAY + 0x10 + (c))
#define GA_COMMWORD_MAIN(c) ((volatile u16 *)GATE_ARRAY + 0x10 + (c) * 2)
#define GA_COMMWORD_MAIN_L(c) ((volatile u32 *)GATE_ARRAY + 0x10 + (c) * 4)
#define GA_COMMWORDS_SUB_B ((volatile u8 *)GATE_ARRAY + 0x20)
#define GA_COMMWORDS_SUB ((volatile u16 *)GATE_ARRAY + 0x20)
#define GA_COMMWORDS_SUB_L ((volatile u32 *)GATE_ARRAY + 0x20)
#define GA_COMMWORD_SUB_B(c) ((volatile u8 *)GATE_ARRAY + 0x20 + (c))
#define GA_COMMWORD_SUB(c) ((volatile u16 *)GATE_ARRAY + 0x20 + (c) * 2)
#define GA_COMMWORD_SUB_L(c) ((volatile u32 *)GATE_ARRAY + 0x20 + (c) * 4)
#if BUS == BUS_MAIN
#define GA_COMMOUT_B GA_COMMWORDS_MAIN_B
#define GA_COMMOUT GA_COMMWORDS_MAIN
#define GA_COMMOUT_L GA_COMMWORDS_MAIN_L
#define GA_COMMIN_B GA_COMMWORDS_SUB_B
#define GA_COMMIN GA_COMMWORDS_SUB
#define GA_COMMIN_L GA_COMMWORDS_SUB_L
#define GA_COMMFLAGS_OUT GA_COMMFLAGS_MAIN
#define GA_COMMFLAGS_IN GA_COMMFLAGS_SUB
#else
#define GA_COMMOUT_B GA_COMMWORDS_SUB_B
#define GA_COMMOUT GA_COMMWORDS_SUB
#define GA_COMMOUT_L GA_COMMWORDS_SUB_L
#define GA_COMMIN_B GA_COMMWORDS_MAIN_B
#define GA_COMMIN GA_COMMWORDS_MAIN
#define GA_COMMIN_L GA_COMMWORDS_MAIN_L
#define GA_COMMFLAGS_OUT GA_COMMFLAGS_SUB
#define GA_COMMFLAGS_IN GA_COMMFLAGS_MAIN
#endif


#define GA_IMASK ((volatile u16 *)GATE_ARRAY + 0x32)
#define GA_IMASK_BL ((volatile u8 *)GATE_ARRAY + 0x33)
#define GA_IM_L1 0x0002
#define GA_IM_L2 0x0004
#define GA_IM_L3 0x0008
#define GA_IM_L4 0x0010
#define GA_IM_L5 0x0020
#define GA_IM_L6 0x0040
#define GA_IM_L1_BIT 1
#define GA_IM_L2_BIT 2
#define GA_IM_L3_BIT 3
#define GA_IM_L4_BIT 4
#define GA_IM_L5_BIT 5
#define GA_IM_L6_BIT 6


#define GA_FADER ((volatile u16 *)GATE_ARRAY + 0x34)
#define GA_FADER_MASK 0x7FF0
#define GA_FADER_SSF 0x0002 // Spindle speed flag
#define GA_FADER_DEF 0x000C // De-emphasis
#define GA_FADER_EFDT 0x8000 // End of fade


#define GA_CDD ((volatile u16 *)GATE_ARRAY + 0x36)
#define GA_CDD_DTS 0x0001 // Data transfer status
#define GA_CDD_DRS 0x0002 // Data receive status
#define GA_CDD_HOCK 0x0004 // Host clock
#define GA_CDD_DM 0x0100 // Data/Muting (1 = mute)


#define GA_CDDCOM ((volatile u16 *)GATE_ARRAY + 0x38) // CDD communication buffers


#define GA_FONTCOLOR ((volatile u16 *)GATE_ARRAY + 0x4C)
#define GA_FONTCOLOR0 0x000F
#define GA_FONTCOLOR1 0x00F0
#define GA_FONTBIT ((volatile u16 *)GATE_ARRAY + 0x4E)
#define GA_FONTDATA ((volatile u16 *)GATE_ARRAY + 0x50)


#define GA_STAMPSIZE ((volatile u16 *)GATE_ARRAY + 0x58)
#define GA_STS 0x0002 // Set : 32x32, Clear : 16x16
#define GA_STS_BIT 1
#define GA_SMS 0x0004 // Set : 16x16, Clear : 1x1
#define GA_SMS_BIT 2
#define GA_RPT 0x0001 // Repeat image if set
#define GA_RPT_BIT 0


#define GA_STMAP ((volatile u16 *)GATE_ARRAY + 0x5A) // Stamp map base address
#define GA_VCELLS ((volatile u16 *)GATE_ARRAY + 0x5C) // Image V cell count minus one
#define GA_IMGBUF ((volatile u16 *)GATE_ARRAY + 0x5E) // Image buffer address
#define GA_IMGOFFSET ((volatile u16 *)GATE_ARRAY + 0x60) // Image buffer offset
#define GA_IMGHSIZE ((volatile u16 *)GATE_ARRAY + 0x62) // Image width in pixels
#define GA_IMGVSIZE ((volatile u16 *)GATE_ARRAY + 0x64) // Image height in pixels
#define GA_TVADDR ((volatile u16 *)GATE_ARRAY + 0x66) // Trace vector address

#define GA_SCADDR ((volatile u16 *)GATE_ARRAY + 0x68) // Subcode address

#include "blsscd_routines.h"

