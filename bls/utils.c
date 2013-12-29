#include "bls.h"

#ifndef BLAST_AS
#define BLAST_AS "m68k-elf-as"
#endif
#ifndef BLAST_CC
#define BLAST_CC "m68k-elf-gcc"
#endif
#ifndef BLAST_CXX
#define BLAST_CXX "m68k-elf-g++"
#endif
#ifndef BLAST_CFLAGS
#define BLAST_CFLAGS "-Os -pipe"
#endif
#ifndef BLAST_LD
#define BLAST_LD "m68k-elf-ld"
#endif
#ifndef BLAST_LDFLAGS
#define BLAST_LDFLAGS ""
#endif
#ifndef BLAST_ASFLAGS
#define BLAST_ASFLAGS ""
#endif
#ifndef BLAST_OBJCOPY
#define BLAST_OBJCOPY "m68k-elf-objcopy"
#endif
#ifndef BLAST_NM
#define BLAST_NM "m68k-elf-nm"
#endif
#ifndef BLAST_ASMX
#define BLAST_ASMX "asmx"
#endif

char cc[1024] = BLAST_CC;
char cxx[1024] = BLAST_CXX;
char cflags[1024] = BLAST_CFLAGS;
char as[1024] = BLAST_AS;
char asflags[1024] = BLAST_ASFLAGS;
char ld[1024] = BLAST_LD;
char ldflags[1024] = BLAST_LDFLAGS;
char objcopy[1024] = BLAST_OBJCOPY;
char nm[1024] = BLAST_NM;
char asmx[1024] = BLAST_ASMX;

void skipblanks(const char **l)
{
  while(**l && **l <= ' ')
  {
    ++*l;
  }
}

int hex2bin(u8 *code, u8 *c)
{
  int bc = 0;
  int b = 0;
  u8 val = 0;
  while(*c)
  {
    if(*c >= 'A' && *c <= 'F')
    {
      val <<= 4;
      val |= *c + 10 - 'A';
      ++b;
    }
    else if(*c >= 'a' && *c <= 'f')
    {
      val <<= 4;
      val |= *c + 10 - 'a';
      ++b;
    }
    else if(*c >= '0' && *c <= '9')
    {
      val <<= 4;
      val |= *c - '0';
      ++b;
    }
    ++c;
    if(b == 2)
    {
      b = 0;
      ++bc;
      *(code++) = (u8)val;
    }
  }
  return bc;
}

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

void filecopy(const char *src, const char *dst)
{
  // TODO : better code needed here
  char cmd[1024];
  snprintf(cmd, 1024, "cp '%s' '%s'", src, dst);
  system(cmd);
}

void fileappend(const char *src, const char *dst)
{
  // TODO : better code needed here
  char cmd[1024];
  snprintf(cmd, 1024, "cat '%s' >> '%s'", src, dst);
  system(cmd);
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

size_t getbasename(char *output, const char *name)
{
  int len = 63;
  char *op = output;

  while(*name) {
    if(*name == '.') {
      len = 0;
    } else if(*name == '/' || *name == '\\') {
      op = output;
      len = 63;
    } else {
      if(len) {
        *op = *name;
        ++op;
        --len;
      }
    }
    ++name;
  }

  *op = '\0';

  return op - output;
}

size_t getext(char *output, const char *name)
{
  int len = 63;
  char *op = output;

  while(*name) {
    if(*name == '/' || *name == '\\' || *name == '.') {
      op = output;
      len = 63;
    } else {
      if(len) {
        *op = *name;
        ++op;
        --len;
      }
    }
    ++name;
  }

  *op = '\0';

  return op - output;
}

size_t getsymname(char *output, const char *path)
{
  int sep;
  char *op = output;

  // Parse first character

  if(!path || !*path)
  {
    printf("Error : Empty path\n");
    exit(1);
  }
  if(*path >= '0' && *path <= '9')
  {
    printf("Warning : symbols cannot start with numbers, prepending underscore to [%s]\n", path);
    *op = '_';
    ++op;
    sep = 0;
  }

  if(*path >= 'a' && *path <= 'z')
  {
    *op = *path - 'a' + 'A';
    ++op;
    ++path;
    sep = 0;
  }
  else if(*path >= 'A' && *path <= 'Z')
  {
    *op = *path;
    ++op;
    ++path;
    sep = 0;
  }
  else
  {
    *op = '_';
    ++op;
    ++path;
    sep = 1;
  }

  while(*path)
  {
    if((*path >= '0' && *path <= '9') || (*path >= 'A' && *path <= 'Z') || *path == '_')
    {
      *op = *path;
      ++op;
      sep = 0;
    }
    else if(*path >= 'a' && *path <= 'z')
    {
      *op = *path - 'a' + 'A';
      ++op;
      sep = 0;
    }
    else
    {
      if(!sep)
      {
        *op = '_';
        ++op;
      }
      sep = 1;
    }
    ++path;
  }
  *op = '\0';

  return op - output;
}

int nextpoweroftwo(int v)
{
  if(v <= 0) return 0;
  v--;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  return v + 1;
}

// Returns 1 if file exists
int fileexists(const char *f)
{
  struct stat s;
  if(stat(f, &s))
  {
    return 0;
  }
  return 1;
}

// Returns modification timestamp for a file
time_t filedate(const char *f)
{
  struct stat s;
  if(stat(f, &s))
  {
    return 0;
  }
  return s.st_mtime;
}

sv_t parse_hex(const char **cp, int len)
{
  skipblanks(cp);
  const char *c = *cp;
  int val = 0;
  while(*c && len--)
  {
    if(*c >= 'A' && *c <= 'F')
    {
      val <<= 4;
      val |= *c + 10 - 'A';
    }
    else if(*c >= 'a' && *c <= 'f')
    {
      val <<= 4;
      val |= *c + 10 - 'a';
    }
    else if(*c >= '0' && *c <= '9')
    {
      val <<= 4;
      val |= *c - '0';
    }
    else
    {
      break;
    }
    ++c;
  }
  *cp = c;

  if(len >= 0) skipblanks(cp);
  return val;
}

void edit_text_file(const char *filename)
{
  char cmdline[1024];
  char *editor;
  if(!(editor = getenv("EDITOR")))
  {
    editor = "vi";
  }
  snprintf(cmdline, 1024, "%s '%s'", editor, filename);
  system(cmdline);
}

sv_t parse_int(const char **cp, int len)
{
  (void)len;
  skipblanks(cp);
  const char *c = *cp;
  int val = 0;
  if(**cp == '$')
  {
    ++*cp;
    return parse_hex(cp, 9);
  }
  if(**cp == '0' && ((*cp)[1] == 'x' || (*cp)[1] == 'X'))
  {
    *cp += 2; 
    return parse_hex(cp, 9);
  }
  while(*c)
  {
    if(*c >= '0' && *c <= '9')
    {
      val *= 10;
      val += *c - '0';
    }
    else
    {
      break;
    }
    ++c;
  }
  *cp = c;
  skipblanks(cp);
  return val;
}

void parse_sym(char *s, const char **cp)
{
  const char *c = *cp;
  while((*c >= 'a' && *c <= 'z') || (*c >= 'A' && *c <= 'Z') || (*c >= '0' && *c <= '9') || *c == '_' || *c == '.')
  {
    *s = *c;
    ++s;
    ++c;
  }
  *s = '\0';
  *cp = c;
}

void hexdump(const u8 *data, int size, u32 offset)
{
  int c;

  while(size)
  {
    // Print line
    printf("%06X: ", offset);
    for(c = 0; c < 16 && size; ++c, ++data, ++offset, --size)
    {
      printf("%02X ", *data);
    }
    printf("\n");
  }
}

