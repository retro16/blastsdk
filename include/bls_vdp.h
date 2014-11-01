#ifndef BLS_VDP_H
#define BLS_VDP_H
#include "bls.h"
#if BUS == BUS_MAIN // VDP is only reachable by main cpu

#define VDPDATA ((volatile u16 *)0xC00000)
#define VDPCTRL ((volatile u16 *)0xC00004)
#define VDPDATA_B ((volatile u8 *)0xC00000)
#define VDPCTRL_B ((volatile u8 *)0xC00004)
#define VDPDATA_L ((volatile u32 *)0xC00000)
#define VDPCTRL_L ((volatile u32 *)0xC00004)

#define VDPREAD 0x00000000
#define VDPWRITE 0x40000000
#define VDPDMA 0x00000080
#define VDPVRAM 0x00000000
#define VDPCRAM 0x80000000
#define VDPVSRAM 0x00000010

#define VDPST_EMPTY 0x0200   // VDP FIFO empty
#define VDPST_FULL 0x0100   // VDP FIFO full
#define VDPST_F 0x0080   // V interrupt happened
#define VDPST_SOVR 0x0040   // Sprite overflow
#define VDPST_C 0x0020   // Sprite collision
#define VDPST_ODD 0x0010   // Odd frame in interlace mode
#define VDPST_VB 0x0008   // 1 during vblank, 0 otherwise
#define VDPST_HB 0x0004   // 1 during hblank, 0 otherwise
#define VDPST_DMA 0x0002   // 1 if DMA is busy
#define VDPST_PAL 0x0001   // 1 if PAL display mode, 0 if NTSC

#define VDPR00 0x04             // VDP register #00
#define VDPHINT 0x10             // Enable HBlank interrupt (level 4)
#define VDPHVEN 0x02             // Enable read HV counter

#define VDPR01 0x04             // VDP register #01
#define VDPDISPEN 0x40             // Display enable
#define VDPVINT 0x20             // Enable VBlank interrupt (level 6)
#define VDPDMAEN 0x10             // Enable DMA
#define VDPV30 0x08             // Display 30 cells vertically (PAL only)

#define VDPR02 (PLANE_A >> 10 & 0x38)   // VDP register #02 - plane a name table
#define VDPR03 0x00                     // VDP register #03 - window name table
#define VDPR04 (PLANE_B >> 13)         // VDP register #04 - plane b name table
#define VDPR05 (SPRAT >> 9 & 0x7E)      // VDP register #05 - sprite attrib table
#define VDPR06 0x00             // VDP register #06
#define VDPR07 0x00             // VDP register #07
#define VDPR08 0x00             // VDP register #08
#define VDPR09 0x00             // VDP register #09
#define VDPR10 0x00             // VDP register #10

#define VDPR11 0x00             // VDP register #11
#define VDPEINT 0x08             // Enable external interrupt (level 2)
#define VDPVCELLSCROLL 0x04             // V scroll : 2 cells
#define VDPHCELLSCROLL 0x02             // H scroll : per cell
#define VDPHLINESCROLL 0x03             // H scroll : per line

#define VDPR12 0x00             // VDP register #12
#define VDPH40 0x81             // Display 40 cells horizontally
#define VDPSTE 0x40             // Enable shadow/hilight
#define VDPILACE 0x02             // Interlace mode
#define VDPILACEDBL 0x06             // Interlace mode (double resolution)


#define VDPR13 (HSCROLL_TABLE >> 10 & 0x3F)     // VDP register #13 - hscroll table
#define VDPR14 0x00             // VDP register #14
#define VDPR15 0x00             // VDP register #15 : autoincrement

#define VDPR16 0x00             // VDP register #16
#define VDPSCRV32 0x00             // VDP scroll 32 cells vertical
#define VDPSCRV64 0x10             // VDP scroll 64 cells vertical
#define VDPSCRV128 0x30             // VDP scroll 128 cells vertical
#define VDPSCRH32 0x00             // VDP scroll 32 cells horizontal
#define VDPSCRH64 0x01             // VDP scroll 64 cells horizontal
#define VDPSCRH128 0x03             // VDP scroll 128 cells horizontal

#define VDPR17 0x00             // VDP register #17
#define VDPWINRIGHT 0x80             // Window on the right hand side

#define VDPR18 0x00             // VDP register #18
#define VDPWINBOT 0x80             // Window at the bottom of the screen


#define VDPCMD(cmd, ram, addr) ((cmd) | (ram) | ( ((addr & 0x3FFF) << 16) | ((addr & 0xC000) >> 14) ))

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

static inline void blsvdp_set_regs(u8 regs[19])
{
  u16 r = 0x8000;
  int i = 0;
  for(i = 0; i < 19; ++i)
  {
    r = (r & 0xFF00 | regs[i]);
    *VDPCTRL = r;
    r += 0x100;
  }
}

static inline void blsvdp_set_autoincrement(u8 incr)
{
  blsvdp_set_reg(15, incr);
}

static inline void blsvdp_enable(int display, int hint, int vint, int dma)
{
  blsvdp_set_reg2(0, VDPR00 | VDPHVEN | (hint ? VDPHINT : 0), 1, VDPR01 | (display ? VDPDISPEN : 0) | (vint ? VDPVINT : 0) | (dma ? VDPDMAEN : 0));
}

// Inline version of DMA copy. Faster for constant values, slower for variables.
// Allows target to be specified (VDPVRAM, VDPCRAM or VDPVSRAM)
// DO NOT do DMA from WRAM on CRAM or VSRAM !
static inline void blsvdp_dma_inline(u32 target, u16 dest, const void *src, u16 len)
{
  blsvdp_set_reg2(19, (len >> 1) & 0xFF, 20, len >> 9);  // Set DMA length
  u32 srcval = (u32)src;
#if TARGET == TARGET_SCD1 || TARGET == TARGET_SCD2

  if(((u32)src & 0x00F00000) == 0x00200000) {
    // WRAM DMA Workaround
    srcval += 2;
  }

#endif
  blsvdp_set_reg2(21, (srcval >> 1) & 0xFF, 22, (srcval >> 9) & 0xFF);
  blsvdp_set_reg2(23, (srcval >> 17) & 0x7F, 15, 2);

  *VDPCTRL = VDPCMD(VDPWRITE | VDPDMA, target, dest) >> 16;

  // Hopefully, the compiler will buffer this variable in RAM
  volatile u16 lastcommand = VDPCMD(VDPWRITE | VDPDMA, target, dest) & 0xFFFF;
  *VDPCTRL = lastcommand;

#if TARGET == TARGET_SCD1 || TARGET == TARGET_SCD2

  if(((u32)src & 0x00F00000) == 0x00200000) {
    // WRAM DMA Workaround
    *VDPCTRL_L = VDPCMD(VDPWRITE, target, dest);
    *VDPDATA = *((const u16 *)src);
  }

#endif
}

static inline void blsvdp_dmafill_inline(u8 data, u16 dest, u16 len) {
  blsvdp_set_reg2(19, len & 0xFF, 20, len >> 8);  // Set DMA length

  blsvdp_set_reg2(23, 0x80, 15, 1); // Set fill mode

  *VDPCTRL_L = VDPCMD(VDPWRITE | VDPDMA, VDPVRAM, dest);
  *VDPDATA_B = data;
}

static inline void blsvdp_send_inline(u32 target, u16 dest, const void *src, u16 len)
{
  blsvdp_set_reg(15, 2);
  *VDPCTRL_L = VDPCMD(VDPWRITE, target, dest);

  u16 i;
  for(i = 0; i < len; i += 2) {
    *VDPDATA = ((const u16 *)src)[i];
  }
}

#endif
#endif
