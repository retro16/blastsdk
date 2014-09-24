#include "blsgen.h"
#include "blsaddress.h"

void fputword(sv value, FILE *f)
{
  fputc((value >> 8) & 0xFF, f);
  fputc((value) & 0xFF, f);
}

void fputlong(sv value, FILE *f)
{
  fputc((value >> 24) & 0xFF, f);
  fputc((value >> 16) & 0xFF, f);
  fputc((value >> 8) & 0xFF, f);
  fputc((value) & 0xFF, f);
}

void fputaddr(chipaddr ca, bus bus, FILE *f)
{
  busaddr ba = chip2bus(ca, bus);
  fputlong(ba.addr, f);
}

// vim: ts=2 sw=2 sts=2 et
