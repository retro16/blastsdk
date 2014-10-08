#ifndef BLSBIPARSE_H
#define BLSBIPARSE_H
#include "bls.h"

extern int getimgtype(const u8 *img, int size);
extern u32 getipoffset(const u8 *img, const u8 **out_ip_start);
extern u32 getspoffset(const u8 *img, const u8 **out_sp_start);
extern u32 detect_region(const u8 *img);

#endif
// vim: ts=2 sw=2 sts=2 et
