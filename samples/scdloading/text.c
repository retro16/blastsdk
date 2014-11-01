#include <bls_vdp.h>

EXTERN_CONST(TEXT_TXT)
#define text EXTERN_DEF(const char *, TEXT_TXT)

EXTERN_CONST(TEXT_TXT_SIZE)
#define textsize EXTERN_DEF(u16, TEXT_TXT_SIZE)

EXTERN_CONST(PLANE_A)
#define vram_plane_a EXTERN_DEF(u16, PLANE_A)

EXTERN_CONST(PLANE_A_SIZE)
#define vram_plane_a_size EXTERN_DEF(u16, PLANE_A_SIZE)

void DISPLAY_TEXT()
{
  u16 lineaddr = vram_plane_a;
  const char *txt = text;
  u16 remaining = textsize;

  blsvdp_set_autoincrement(2);
  while(remaining && lineaddr < vram_plane_a + vram_plane_a_size) {
    *VDPCTRL_L = VDPCMD(VDPWRITE, VDPVRAM, lineaddr);
    u16 linelen = 0x40;
    while(remaining && *txt != '\n') {
      *VDPDATA = (u16)*txt;
      ++txt;
      --remaining;
      --linelen;
      if(linelen == 0x40 - 40) {
        linelen = 0x40;
        lineaddr += 0x80;
        if(lineaddr >= vram_plane_a + vram_plane_a_size) {
          return; // No more room on screen
        }
      }
    }
    if(remaining) {
      // Skip line break
      ++txt;
      --remaining;
    }
    while(linelen > 0) {
      // Clear after end of line
      *VDPDATA = (u16)' ';
      --linelen;
    }
    lineaddr += 0x80;
  }

  // Clear the screen after last line if needed
  while(lineaddr < vram_plane_a + vram_plane_a_size) {
    *VDPCTRL_L = VDPCMD(VDPWRITE, VDPVRAM, lineaddr);
    for(remaining = 0, remaining < linelen; ++remaining) {
      *VDPDATA = (u16)' ';
    }
    lineaddr += 0x80;
  }
}
