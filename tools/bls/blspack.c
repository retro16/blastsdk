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

const char * sections_cat(group *bin, const char *binname)
{
  FILE *i, *o;
  o = fopen(binname, "wb");

  BLSLL(section) *secl = bin->provides;
  section *sec;
  BLSLL_FOREACH(sec, secl) {
    // Ignore null sections
    if(sec->physsize == 0 || sec->format == format_zero || sec->format == format_empty) {
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

  bin->physsize = ftell(o);
  fclose(o);

  return binname;
}
