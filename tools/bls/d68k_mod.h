#ifndef D68K_MOD_H
#define D68K_MOD_H

#include "blsaddress.h"

/* 68000 disassembler.
 */

// Errors returned by d68k()
extern const char *d68k_error(int64_t r);
extern int64_t d68k(char *_targetdata, int _tsize, const u8 *_code, int _size, int _instructions, u32 _address, int _labels, int _showcycles, int *_suspicious);
extern void d68k_readsymbols(const char *filename);
extern void d68k_freesymbols();
extern void d68k_setbus(bus b);
extern const char * getdsymat(u32 addr);
extern sv getdsymval(const char *name);

#endif
// vim: ts=2 sw=2 sts=2 et
