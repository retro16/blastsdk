#include "bls.h"

// Returns the type of the image
// 0 = invalid
// 1 = genesis
// 2 = CD
// 3 = SCD WRAM program
// 4 = Genesis RAM program
int getimgtype(const u8 *img, int size)
{
  if(size < 0x200)
  {
    return 0;
  }
  if(strncmp((const char *)img, "SEGA", 4) == 0)
  {
    return 2;
  }
  if(strncmp((const char *)img + 0x100, "SEGA", 4) == 0)
  {
    // Cart image
    if(img[5] == 0x20)
    {
      return 3;
    }
    if(img[5] == 0xFF)
    {
      return 4;
    }
    return 1;
  }

  return 0;
}

// Returns the offset and size of IP (skipping header and security code)
int getipoffset(const u8 *img, const u8 **out_ip_start)
{
  if(img[0x22] == 0x02 && img[0x23] == 0x00)
  {
    // Small IP
    *out_ip_start = img + CDHEADERSIZE + detect_region(img);
    return 0x600;
  } else {
    // Big IP
    *out_ip_start = img + CDHEADERSIZE + detect_region(img);
    return 0x8000 - (*out_ip_start - img);
  }
}

int getspoffset(const u8 *img, const u8 **out_sp_start)
{
  *out_sp_start = img + getint(&img[0x40], 4) + SPHEADERSIZE;
  return getint(&img[0x44], 4) - SPHEADERSIZE;
}

// Returns security code length
int detect_region(const u8 *img)
{
  switch(img[CDHEADERSIZE + 11])
  {
    case 0x7a:  return 0x584; // US
    case 0x64: return 0x56e; // EU
    case 0xa1: return 0x156; // JP
  }
  printf("Warning : unknown CD region.\n");
  return 0;
}
