#include "bls.h"
#include <string.h>
#include <stdio.h>

int getimgtype(const u8 *img, int size);
u32 getipoffset(const u8 *img, const u8 **out_ip_start);
u32 getspoffset(const u8 *img, const u8 **out_sp_start);
u32 getspentry(const u8 *img);
u32 getspl2(const u8 *img);
u32 detect_region(const u8 *img);

// Returns the type of the image
// Needs at least 0x200 bytes
//
// 0 = invalid
// 1 = genesis
// 2 = SCD ISO
// 3 = Virtual cart
// 4 = RAM program
//
int getimgtype(const u8 *img, int size)
{
  if(size < 0x200) {
    return 0;
  }

  if(strncmp((const char *)img, "SEGA", 4) == 0) {
    return 2;
  }

  if(strncmp((const char *)img + 0x100, "SEGA VIRTUALCART", 16) == 0) {
    return 3;
  }

  if(strncmp((const char *)img + 0x100, "SEGA RAM PROGRAM", 16) == 0) {
    return 4;
  }

  if(strncmp((const char *)img + 0x100, "SEGA", 4) == 0) {
    return 1;
  }

  return 0;
}

// Returns the offset and size of IP (skipping header and security code)
u32 getipoffset(const u8 *img, const u8 **out_ip_start)
{
  if(img[0x22] == 0x02 && img[0x23] == 0x00) {
    // Small IP
    *out_ip_start = img + CDHEADERSIZE + detect_region(img);
    return 0x600;
  } else {
    // Big IP
    *out_ip_start = img + CDHEADERSIZE + detect_region(img);
    return 0x8000 - (*out_ip_start - img);
  }
}

u32 getspoffset(const u8 *img, const u8 **out_sp_start)
{
  *out_sp_start = img + getint(&img[0x40], 4);
  return getint(&img[0x44], 4);
}

u32 getspinit(const u8 *img)
{
  const u8 *spoffset;
  getspoffset(img, &spoffset);

  u32 table = 0x6000 + getint(spoffset + 0x18, 2);

  return table + getint(table, 2);
}

u32 getspmain(const u8 *img)
{
  const u8 *spoffset;
  getspoffset(img, &spoffset);

  u32 table = 0x6000 + getint(spoffset + 0x18, 2);

  return table + getint(table + 2, 2);
}

u32 getspl2(const u8 *img)
{
  const u8 *spoffset;
  getspoffset(img, &spoffset);

  u32 table = 0x6000 + getint(spoffset + 0x18, 2);

  return table + getint(table + 4, 2);
}

// Returns security code length
u32 detect_region(const u8 *img)
{
  switch(img[CDHEADERSIZE + 11]) {
  case 0x7a:  return 0x584; // US

  case 0x64: return 0x56e; // EU

  case 0xa1: return 0x156; // JP
  }

  printf("Warning : unknown CD region.\n");
  return 0;
}

// vim: ts=2 sw=2 sts=2 et
