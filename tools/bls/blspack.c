#include "blspack.h"
#include "blsfile.h"

static void pack_open(const char *filename, FILE **i, FILE **o)
{
  char iname[4096];
  char oname[4096];
  snprintf(iname, 4096, BUILDDIR"/%s", filename);
  snprintf(oname, 4096, BUILDDIR"/%s.phy", filename);

  *i = fopen(iname, "rb");

  if(!*i) {
    printf("Could not open %s for input\n", iname);
    exit(1);
  }

  *o = fopen(oname, "wb");

  if(!*o) {
    printf("Could not open %s for output\n", oname);
    exit(1);
  }
}

size_t pack_raw(const char *filename)
{
  FILE *i, *o;
  pack_open(filename, &i, &o);

  // Copy the file without transform
  size_t filesize = filecat(i, o);

  fclose(i);
  fclose(o);

  return filesize;
}

const char *sections_cat(group *bin, const char *binname)
{
  FILE *i, *o;
  o = fopen(binname, "wb");

  BLSLL(section) *secl = bin->provides;
  section *sec;

  if(bin == mainout.ipbin || bin == mainout.spbin) {
    sv minaddr = 0xFFFFFFFF;
    sv maxaddr = 0;

    // Compute maxaddr and minaddr
    BLSLL_FOREACH(sec, secl) {
      // Ignore null sections
      if(sec->size == 0 || sec->format != format_raw) {
        continue;
      }
      if(sec->symbol->value.addr < minaddr) {
        minaddr = sec->symbol->value.addr;
      }
      if(sec->symbol->value.addr + sec->size > maxaddr) {
        maxaddr = sec->symbol->value.addr + sec->size;
      }
    }

    printf("minaddr=%X maxaddr=%X\n", (u32)minaddr, (u32)maxaddr);

    // Reserve space in the output file for the whole binary
    sv binoffset = ftell(o);
    sv c;
    for(c = 0; c < maxaddr - minaddr; ++c) {
      fputc('\0', o);
    }

    // Copy sections in their correct offset directly in the file
    secl = bin->provides;
    BLSLL_FOREACH(sec, secl) {
      // Ignore null sections
      if(sec->size == 0 || sec->format != format_raw) {
        continue;
      }

      printf("%s : %X\n", sec->name, (u32)(binoffset + sec->symbol->value.addr - minaddr));
      fseek(o, binoffset + sec->symbol->value.addr - minaddr, SEEK_SET);

      char secfile[4096];
      snprintf(secfile, 4096, BUILDDIR"/%s", sec->name);
      sec->physaddr = ftell(o);
      i = fopen(secfile, "rb");
      filecat(i, o);
      sec->physsize = ftell(i);
    }

    // Move cursor after the binary
    fseek(o, binoffset + maxaddr - minaddr, SEEK_SET);
  } else {
    BLSLL_FOREACH(sec, secl) {
      // Ignore null sections
      if(sec->physsize == 0 || sec->format == format_zero || sec->format == format_empty) {
        sec->physsize = 0;
        continue;
      }

      char secfile[4096];
      snprintf(secfile, 4096, BUILDDIR"/%s", sec->name);
      sec->physaddr = ftell(o);
      i = fopen(secfile, "rb");
      filecat(i, o);
      sec->physsize = ftell(i);
      if(sec->physsize & 1) {
        fputc('\x00', o); // Align sections to word in binary
      }

      fclose(i);
    }
  }

  bin->physsize = ftell(o);
  fclose(o);

  return binname;
}

// vim: ts=2 sw=2 sts=2 et
