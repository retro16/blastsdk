#include "blspack.h"

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

static size_t filecat(FILE *i, FILE *o)
{
  // Copy a file and returns its size
  size_t filesize = 0;
  while(!feof(i)) {
    char buf[65536];
    size_t r = fread(buf, 1, sizeof(buf), i);
    filesize += r;
    fwrite(buf, 1, r, o);
  }

  return filesize;
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

size_t pack_lzbyte(const char *filename)
{
  FILE *i, *o;
  pack_open(filename, &i, &o);

  // TODO
  size_t filesize = 0;
  while(!feof(i)) {
    char buf[65536];
    size_t r = fread(buf, 1, sizeof(buf), i);
    filesize += r;
    fwrite(buf, 1, r, o);
  }

  fclose(i);
  fclose(o);

  return filesize;
}

size_t pack_lzword(const char *filename)
{
  FILE *i, *o;
  pack_open(filename, &i, &o);

  // TODO
  size_t filesize = 0;
  while(!feof(i)) {
    char buf[65536];
    size_t r = fread(buf, 1, sizeof(buf), i);
    filesize += r;
    fwrite(buf, 1, r, o);
  }

  fclose(i);
  fclose(o);

  return filesize;
}

const char * sections_cat(group *bin)
{
  static char binname[4096];
  snprintf(binname, 4096, BUILDDIR"/%s.bin");
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
    i = fopen(secfile, "rb");
    filecat(i, o);
    fclose(i);
  }

  return binname;
}
