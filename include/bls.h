/* Blast! SDK main include
 */
#ifndef BLS_H
#define BLS_H

typedef unsigned char u8
typedef unsigned short u16
typedef unsigned long u32

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
#define TARGET_GEN 0
#define TARGET_SCD 1
#define TARGET_RAM 2

#define TRAP(t) asm volatile("\tTRAP #"#t)
#define BLS_INT_CALLBACK __attribute__((interrupt))
typedef void (*bls_int_callback)();

#define bls_enable_interrupts() asm volatile("\tandi #$F8FF, SR\n")
#define bls_disable_interrupts() asm volatile("\tori #$0700, SR\n")

#if BUS == BUS_MAIN
#define ENTER_MONITOR() TRAP(7)

#if TARGET == TARGET_GEN
#define VDPSECURITY() \
asm volatile("\tmove.b %d0, -(%a7)\n" \
"\tmove.b 0xA10001, %d0\n" \
"\tandi.b #0x0F, %d0\n" \
"\tbeq.b 1f\n" \
"\tmove.l 0x100, 0xA14000\n" \
"1:\tmove.b (%a7)+, %d0\n" )
#else
#define VDPSECURITY() do {} while(0)
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
      ++(u32 *)dest;
      ++(u32 *)src;
    case 28:
      *(u32 *)dest = *(u32 *)src;
      ++(u32 *)dest;
      ++(u32 *)src;
    case 24:
      *(u32 *)dest = *(u32 *)src;
      ++(u32 *)dest;
      ++(u32 *)src;
    case 20:
      *(u32 *)dest = *(u32 *)src;
      ++(u32 *)dest;
      ++(u32 *)src;
    case 16:
      *(u32 *)dest = *(u32 *)src;
      ++(u32 *)dest;
      ++(u32 *)src;
    case 12:
      *(u32 *)dest = *(u32 *)src;
      ++(u32 *)dest;
      ++(u32 *)src;
    case 8:
      *(u32 *)dest = *(u32 *)src;
      ++(u32 *)dest;
      ++(u32 *)src;
    case 4:
      *(u32 *)dest = *(u32 *)src;
      ++(u32 *)dest;
      ++(u32 *)src;
    default:
      break;
  }
  if(length & 2) {
    *(u16 *)dest = *(u16 *)src;
    ++(u16 *)dest;
    ++(u16 *)src;
  }
  if(length & 1) {
    *(u8 *)dest = *(u8 *)src;
    ++(u8 *)dest;
    ++(u8 *)src;
  }
}


#endif // BSS_MAIN

#endif
