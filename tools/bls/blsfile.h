#include "bls.h"
#include <stdio.h>

extern int readfile(const char *name, u8 **data);
extern int writefile(const char *name, const u8 *data, int size);
extern size_t filecat(FILE *i, FILE *o);
extern void filecopy(const char *src, const char *dst);
extern void fileappend(const char *src, const char *dst);
extern size_t filesize(const char *file);

// vim: ts=2 sw=2 sts=2 et
