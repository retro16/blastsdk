#ifndef BLSWRITE_H
#define BLSWRITE_H

void fputword(sv value, FILE *f);
void fputlong(sv value, FILE *f);
void fputaddr(chipaddr ca, bus bus, FILE *f);

#endif
// vim: ts=2 sw=2 sts=2 et
