#ifndef BLSPACK_H
#define BLSPACK_H

#include "blsgen.h"

size_t filecat(FILE *i, FILE *o);

size_t pack_raw(const char *filename);
size_t pack_lzbyte(const char *filename);
size_t pack_lzword(const char *filename);

const char *sections_cat(group *bin, const char *binname);

#endif
// vim: ts=2 sw=2 sts=2 et
