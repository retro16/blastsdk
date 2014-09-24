#ifndef BLSPARSE_H
#define BLSPARSE_H

#include "bls.h"

extern int hex2bin(u8 *code, u8 *c);
extern void skipblanks(const char **l);
extern int64_t parse_hex_skip(const char **cp);
extern int64_t parse_hex(const char *cp);
extern int64_t parse_int_skip(const char **cp);
extern int64_t parse_int(const char *cp);
extern void parse_sym(char *s, const char **cp);
extern size_t getsymname(char *output, const char *path);
extern void hexdump(const u8 *data, int size, u32 offset);

#endif

// vim: ts=2 sw=2 sts=2 et
