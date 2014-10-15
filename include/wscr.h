#ifndef WSCR_H_INCL
#define WSCR_H_INCL

#include "bls_vdp.h"
#include "bls_init.h"

#define WSCR_W 256
#define WSCR_H 152
#define WSCR_TOPLINE 16
#define WSCR_BOTLINE (WSCR_TOPLINE + WSCR_H)

#define WSCR_PAD_UP 0x0001
#define WSCR_PAD_DOWN 0x0002
#define WSCR_PAD_LEFT 0x0004
#define WSCR_PAD_RIGHT 0x0008
#define WSCR_PAD_A 0x1000
#define WSCR_PAD_B 0x0010
#define WSCR_PAD_C 0x0020
#define WSCR_PAD_START 0x2000

#define WSCR_PAD_UP_BIT 0
#define WSCR_PAD_DOWN_BIT 1
#define WSCR_PAD_LEFT_BIT 2
#define WSCR_PAD_RIGHT_BIT 3
#define WSCR_PAD_A_BIT 12
#define WSCR_PAD_B_BIT 4
#define WSCR_PAD_C_BIT 5
#define WSCR_PAD_START_BIT 13

#define WSCR_PAD_STATUS GA_COMMWORD_MAIN_L(0)
#define WSCR_PAD1_STATUS GA_COMMWORD_MAIN(0)
#define WSCR_PAD2_STATUS GA_COMMWORD_MAIN(1)
#define WSCR_MISS_COUNT GA_COMMWORD_MAIN_W(2)

extern u16 WSCR_FBUF_PLANE;
extern u16 WSCR_HSCROLL;


extern void _MAIN_WSCR_INIT_VDP(u16 tile_attr, u16 (*vblank_callback)(), bls_int_callback vblank_miss_callback);
static inline void wscr_init_vdp(u16 (*vblank_callback)(), bls_int_callback vblank_miss_callback, u16 tile_attr, u16 plane_a_addr, u16 sprites_addr, u16 interlace) {
  _MAIN_WSCR_INIT_VDP(tile_attr, vblank_callback, vblank_miss_callback);
  u8 regs[19] = {
    VDPR00 | VDPHINT | VDPHVEN,
    VDPR01 | VDPDMAEN,
    plane_a_addr >> 10 & 0x38,
    0,
    wscr_fbuf_plane >> 13,
    sprites_addr >> 9 & 0x7E,
    VDPR06,
    VDPR07,
    VDPR08,
    VDPR09,
    VDPR10 | WSCR_BOTLINE,
    VDPR11 | VDPEINT,
    VDPR12 | VDPH40 | interlace,
    WSCR_HSCROLL >> 10 & 0x3F,
    VDPR14,
    VDPR15 | 2,
    VDPR16 | VDPSCRH64 | VDPSCRV32,
    0,
    0
  };
  blsvdp_set_regs(regs);
#if TARGET == TARGET_SCD2
  send_wram_2m();
#endif
}

#endif
