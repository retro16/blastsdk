#ifndef BLS_INIT_H
#define BLS_INIT_H

extern void BLS_INIT(bls_int_callback hint, bls_int_callback vint);
static inline void bls_init(bls_int_callback hint, bls_int_callback vint) {
  BLS_INIT(hint, vint);
}

#define VDPWIN_TOPLEFT 0x0000
#define VDPWIN_TOPRIGHT 0x8000
#define VDPWIN_BOTLEFT 0x0080
#define VDPWIN_BOTRIGHT 0x8080

static inline void bls_init_vdp(int hint, int vint, int plane_w, int plane_h, u32 plane_a_addr, u32 window_addr, u32 plane_b_addr, u32 sprites_addr, u32 hscroll_addr, int hscroll_mode, int vscroll_mode, int window_x, int window_y, int shadow, int interlace)
{
  u8 regs[19] = {
    VDPR00 | (hint ? VDPHINT : 0),
    VDPR01 | VDPDISPEN | (vint ? VDPVINT : 0) | VDPDMAEN,
    plane_a_addr >> 10 & 0x38,
    window_addr >> 10 & 0x3E,
    plane_b_addr >> 13,
    sprites_addr >> 9 & 0x7E,
    VDPR06,
    VDPR07,
    VDPR08,
    VDPR09,
    VDPR10,
    VDPR11 | VDPEINT | hscroll_mode | vscroll_mode,
    VDPR12 | VDPH40 | (shadow?VDPSTE:0) | interlace,
    hscroll_addr >> 10 & 0x3F,
    VDPR14,
    VDPR15 | 2,
    VDPR16 | (plane_w==32 ? VDPSCRH32 : plane_w==64 ? VDPSCRH64 : VDPSCRH128) | (plane_h==32 ? VDPSCRV32 : plane_h==64 ? VDPSCRV64 : VDPSCRV128),
    (window_pos >> 8 & $FF) | window_x,
    (window_pos & $FF) | window_y
  };
  blsvdp_set_regs(regs);
}

#endif
