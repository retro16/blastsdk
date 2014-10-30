/* Blast! SDK main include
 */
#ifndef BLS_H
#define BLS_H

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;

#define EXTERN_CONST(name) extern void *name;
#define EXTERN_DEF(type, name) ((type)(u32)&name)

// Bus constants
#define BUS_NONE 0
#define BUS_MAIN 1
#define BUS_SUB 2
#define BUS_Z80 3

// Chip constants
#define CHIP_NONE 0
#define CHIP_MSTACK 1
#define CHIP_SSTACK 2
#define CHIP_ZSTACK 3
#define CHIP_CART 4
#define CHIP_BRAM 5
#define CHIP_ZRAM 6
#define CHIP_VRAM 7
#define CHIP_RAM 8
#define CHIP_PRAM 9
#define CHIP_WRAM 10
#define CHIP_PCM 11

// Target constants
#define TARGET_UNKNOWN 0
#define TARGET_GEN 1
#define TARGET_SCD1 2
#define TARGET_SCD2 2
#define TARGET_RAM 4

#if BUS == BUS_MAIN
// Game controller lines
#define CUP 0x01
#define CUP_BIT 0
#define CDOWN 0x02
#define CDOWN_BIT 1
#define CLEFT 0x04
#define CLEFT_BIT 2
#define CRIGHT 0x08
#define CRIGHT_BIT 3
#define CTL 0x10
#define CTL_BIT 4
#define CTR 0x20
#define CTR_BIT 5
#define CTH 0x40
#define CTH_BIT 6
#define CSEL CTH
#define CSEL_BIT CTH_BIT
#define CSEL_ASTART 0
#define CSEL_LRBC CSEL
#define CBTNB CTL
#define CBTNB_BIT CTL_BIT
#define CBTNA CTL
#define CBTNA_BIT CTL_BIT
#define CBTNC CTR
#define CBTNC_BIT CTR_BIT
#define CBTNSTART CTR
#define CBTNSTART_BIT CTR_BIT

// CCTRL bits
#define CTHINT 0x80 // Enable TH interrupt

#define CDATA1 ((volatile u8 *)0xA1003)
#define CDATA2 ((volatile u8 *)0xA1005)
#define CDATA3 ((volatile u8 *)0xA1007)

#define CCTRL1 ((volatile u8 *)0xA1009)
#define CCTRL2 ((volatile u8 *)0xA100B)
#define CCTRL3 ((volatile u8 *)0xA100D)

#define STX1 ((volatile u8 *)0xA1000F)
#define SRX1 ((volatile u8 *)0xA10011)
#define SCTRL1 ((volatile u8 *)0xA10013)
#define STX2 ((volatile u8 *)0xA10015)
#define SRX2 ((volatile u8 *)0xA10017)
#define SCTRL2 ((volatile u8 *)0xA10019)
#define STX3 ((volatile u8 *)0xA1001B)
#define SRX3 ((volatile u8 *)0xA1001D)
#define SCTRL3 ((volatile u8 *)0xA1001F)

// SCTRL bits
#define SCTFULL 0x01
#define SCRRDY 0x02
#define SCRERR 0x04
#define SCRINT 0x08
#define SCSOUT 0x10
#define SCSIN 0x20
#define SC300 0xC0
#define SC1200 0x80
#define SC2400 0x40
#define SC4800 0x00

#define ZBUSREQ ((volatile u16 *)0xA11100)
#define ZRESET ((volatile u16 *)0xA11200)
#define ZREQ 0x0100

// Sound defines
#if BUS == BUS_Z80
#define FM1REG ((volatile u8 *)0x4000)
#define FM1DATA ((volatile u8 *)0x4001)
#define FM2REG ((volatile u8 *)0x4002)
#define FM2DATA ((volatile u8 *)0x4003)
#define PSG ((volatile u8 *)0x7F11)
#else
#define FM1REG ((volatile u8 *)0xA04000)
#define FM1DATA ((volatile u8 *)0xA04001)
#define FM2REG ((volatile u8 *)0xA04002)
#define FM2DATA ((volatile u8 *)0xA04003)
#define PSG ((volatile u8 *)0xC00011)
#endif

#endif // BUS == BUS_MAIN

// Interrupts

#define BLS_INT_CALLBACK __attribute__((interrupt))
typedef void (*bls_int_callback)();
typedef bls_int_callback *bls_int_vector;

#define INTV_BUSERR ((bls_int_vector)0x08)
#define INTV_ADDRERR ((bls_int_vector)0x0C)
#define INTV_ILL ((bls_int_vector)0x10)
#define INTV_ZDIV ((bls_int_vector)0x14)
#define INTV_CHK ((bls_int_vector)0x18)
#define INTV_TRAPV ((bls_int_vector)0x1C)
#define INTV_PRIV ((bls_int_vector)0x20)
#define INTV_TRACE ((bls_int_vector)0x24)
#define INTV_LINEA ((bls_int_vector)0x28)
#define INTV_LINEF ((bls_int_vector)0x2C)

#define INTV_SPURIOUS ((bls_int_vector)0x60)
#define INTV_LEVEL1 ((bls_int_vector)0x64)
#define INTV_LEVEL2 ((bls_int_vector)0x68)
#define INTV_PAD INTV_LEVEL2
#define INTV_LEVEL3 ((bls_int_vector)0x6C)
#define INTV_LEVEL4 ((bls_int_vector)0x70)
#define INTV_HBLANK INTV_LEVEL4
#define INTV_LEVEL5 ((bls_int_vector)0x74)
#define INTV_LEVEL6 ((bls_int_vector)0x78)
#define INTV_VBLANK INTV_LEVEL6
#define INTV_LEVEL7 ((bls_int_vector)0x7C)

#define INTV_TRAP00 ((bls_int_vector)0x80)
#define INTV_TRAP01 ((bls_int_vector)0x84)
#define INTV_TRAP02 ((bls_int_vector)0x88)
#define INTV_TRAP03 ((bls_int_vector)0x8C)
#define INTV_TRAP04 ((bls_int_vector)0x90)
#define INTV_TRAP05 ((bls_int_vector)0x94)
#define INTV_TRAP06 ((bls_int_vector)0x98)
#define INTV_TRAP07 ((bls_int_vector)0x9C)
#define INTV_TRAP08 ((bls_int_vector)0xA0)
#define INTV_TRAP09 ((bls_int_vector)0xA4)
#define INTV_TRAP10 ((bls_int_vector)0xA8)
#define INTV_TRAP11 ((bls_int_vector)0xAC)
#define INTV_TRAP12 ((bls_int_vector)0xB0)
#define INTV_TRAP13 ((bls_int_vector)0xB4)
#define INTV_TRAP14 ((bls_int_vector)0xB8)
#define INTV_TRAP15 ((bls_int_vector)0xBC)



#if BUS == BUS_MAIN

#if TARGET == TARGET_GEN
static inline void VDPSECURITY() {
  asm volatile("\tmove.b %d0, -(%a7)\n"
  "\tmove.b 0xA10001, %d0\n"
  "\tandi.b #0x0F, %d0\n"
  "\tbeq.b 1f\n"
  "\tmove.l 0x100, 0xA14000\n"
  "1:\tmove.b (%a7)+, %d0\n" );
}
#else
static inline void VDPSECURITY() {}
#endif

// Fast inline copy not using stdlib.asm. Optimized for constant length.
static inline void blscopy_inline(void *dest, const void *src, u32 length)
{
  if(((u32)dest & 1) || ((u32)src & 1)) {
    u8 *d8 = (u8 *)dest;
    u8 *s8 = (u8 *)src;
    while(length--) {
      *d8 = *s8;
      ++s8;
      ++d8;
    }
    return;
  }

  while(length > 32) {
    ((u32 *)dest)[0] = ((u32 *)src)[0];
    ((u32 *)dest)[1] = ((u32 *)src)[1];
    ((u32 *)dest)[2] = ((u32 *)src)[2];
    ((u32 *)dest)[3] = ((u32 *)src)[3];
    ((u32 *)dest)[4] = ((u32 *)src)[4];
    ((u32 *)dest)[5] = ((u32 *)src)[5];
    ((u32 *)dest)[6] = ((u32 *)src)[6];
    ((u32 *)dest)[7] = ((u32 *)src)[7];
    length -= 32;
  }

  switch((u8)length & 0xFC) {
    case 32:
      *(u32 *)dest = *(u32 *)src;
      dest = &((u32 *)dest)[1];
      src = &((u32 *)src)[1];
    case 28:
      *(u32 *)dest = *(u32 *)src;
      dest = &((u32 *)dest)[1];
      src = &((u32 *)src)[1];
    case 24:
      *(u32 *)dest = *(u32 *)src;
      dest = &((u32 *)dest)[1];
      src = &((u32 *)src)[1];
    case 20:
      *(u32 *)dest = *(u32 *)src;
      dest = &((u32 *)dest)[1];
      src = &((u32 *)src)[1];
    case 16:
      *(u32 *)dest = *(u32 *)src;
      dest = &((u32 *)dest)[1];
      src = &((u32 *)src)[1];
    case 12:
      *(u32 *)dest = *(u32 *)src;
      dest = &((u32 *)dest)[1];
      src = &((u32 *)src)[1];
    case 8:
      *(u32 *)dest = *(u32 *)src;
      dest = &((u32 *)dest)[1];
      src = &((u32 *)src)[1];
    case 4:
      *(u32 *)dest = *(u32 *)src;
      dest = &((u32 *)dest)[1];
      src = &((u32 *)src)[1];
    default:
      break;
  }
  if(length & 2) {
    *(u16 *)dest = *(u16 *)src;
    dest = &((u16 *)dest)[1];
    src = &((u16 *)src)[1];
  }
  if(length & 1) {
    *(u8 *)dest = *(u8 *)src;
    dest = &((u8 *)dest)[1];
    src = &((u8 *)src)[1];
  }
}


#endif // BUS_MAIN

#include "bls_routines.h"

#if TARGET == TARGET_SCD1 || TARGET == TARGET_SCD2
#include "blsscd.h"
#endif

#include "bls_vdp.h"

#endif
