#ifndef BLS_VDP_H
#define BLS_VDP_H
#include "bls.h"
#if BUS == BUS_MAIN // VDP is only reachable by main cpu

#define VDPDATA ((volatile u16 *)0xC00000)
#define VDPCTRL ((volatile u16 *)0xC00004)
#define VDPDATA_L ((volatile u32 *)0xC00000)
#define VDPCTRL_L ((volatile u32 *)0xC00004)

#define VDPREAD 0x00000000
#define VDPWRITE 0x40000000
#define VDPDMA 0x00000080
#define VDPVRAM 0x80000000
#define VDPVSRAM 0x00000010

#define VDPCMD(cmd, target, addr) ((cmd) | (target) | ((addr) & 0x00003FFF) | (((addr) << 2) & 0x00030000))

// Defined in bls_vdp.asm
extern void blsvdp_dma(const void *dest, const void *src, unsigned int len);

static inline void blsvdp_set_reg(u8 reg, u8 value)
{
  *VDPCTRL = ((reg + 0x80) << 8) | value;
}

static inline void blsvdp_set_reg2(u8 reg1, u8 value1, u8 reg2, u8 value2)
{
  *VDPCTRL_L = ((reg1 + 0x80) << 24) | (value1 << 16) | ((reg2 + 0x80) << 8) | value2;
}

static inline void blsvdp_set_autoincrement(u8 incr)
{
  blsvdp_set_reg(15, incr);
}


// Inline version of DMA copy. Faster for constant values, slower for variables.
// Allows target to be specified (VDPVRAM, VDPCRAM or VDPVSRAM)
// DO NOT do DMA from WRAM on CRAM or VSRAM !
static inline void blsvdp_dma_inline(u32 target, u16 dest, const void *src, u16 len)
{
  blsvdp_set_reg2(19, len & 0xFF, 20, len >> 8);  // Set DMA length
  u32 srcval = ((u32)src) / 2;
#if TARGET == TARGET_SCD1 || TARGET == TARGET_SCD2

  if((u8)(((u32)src) >> 16) == 0x20) {
    // WRAM DMA Workaround
    srcval += 2;
  }

#endif
  blsvdp_set_reg2(21, srcval & 0xFF, 22, (srcval >> 8) & 0xFF);
  blsvdp_set_reg(23, (srcval >> 16) & 0xFF);

  *VDPCTRL = VDPCMD(VDPWRITE | VDPDMA, target, dest) >> 16;

  // Hopefully, the compiler will buffer this variable in RAM
  volatile u16 lastcommand = VDPCMD(VDPWRITE | VDPDMA, target, dest) & 0xFFFF;
  *VDPCTRL = lastcommand;

#if TARGET == TARGET_SCD1 || TARGET == TARGET_SCD2

  if((u8)(((u32)src) >> 16) == 0x20) {
    // WRAM DMA Workaround
    *VDPCTRL_L = VDPCMD(VDPWRITE, target, dest);
    *VDPDATA = *((const u16 *)src);
  }

#endif
}

static inline void blsvdp_send_inline(u32 target, u16 dest, const void *src, u16 len)
{
  *VDPCTRL_L = VDPCMD(VDPWRITE, target, dest);

  for(u16 i = 0; i < len; i += 2) {
    *VDPDATA = ((const u16 *)src)[i];
  }
}

#endif
#endif
