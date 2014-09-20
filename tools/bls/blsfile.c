#include "bls.h"
#include <stdio.h>

int readfile(const char *name, u8 **data);
int writefile(const char *name, const u8 *data, int size);
size_t filecat(FILE *i, FILE *o);
void filecopy(const char *src, const char *dst);
void fileappend(const char *src, const char *dst);
size_t filesize(const char *file);

int readfile(const char *name, u8 **data)
{
  FILE *f = fopen(name, "r");
  if(!f)
  {
    return -1;
  }
  fseek(f, 0, SEEK_END);
  size_t s = ftell(f);
  fseek(f, 0, SEEK_SET);

  *data = malloc(s);
  int r = fread(*data, 1, s, f);

  fclose(f);
  return r;
}

int writefile(const char *name, const u8 *data, int size)
{
  FILE *f = fopen(name, "w");
  if(!f)
  {
    return 0;
  }

  fwrite(data, 1, size, f);

  fclose(f);

  return 1;
}

size_t filecat(FILE *i, FILE *o)
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

void filecopy(const char *src, const char *dst)
{
  FILE *i;
  FILE *o;

  i = fopen(src, "rb");
  if(!i) return;

  o = fopen(dst, "wb");
  if(o)
  {
    filecat(i, o);
  }

  fclose(i);
  fclose(o);
}

void fileappend(const char *src, const char *dst)
{
  FILE *i;
  FILE *o;

  i = fopen(src, "rb");
  if(!i) return;

  o = fopen(dst, "ab");
  if(o)
  {
    filecat(i, o);
  }

  fclose(i);
  fclose(o);
}

size_t filesize(const char *file)
{
  FILE *f = fopen(file, "rb");
  if(!f) {
    printf("Cannot open file %s to get its size.\n", file);
    exit(1);
  }

  fseek(f, 0, SEEK_END);
  size_t r = ftell(f);
  fclose(f);
  return r;
}

// Returns 1 if file exists
int fileexists(const char *f)
{
  FILE *f = fopen(f, "rb");
  if(!f) {
    return 0;
  }
  fclose(f);
  return 1;
}
