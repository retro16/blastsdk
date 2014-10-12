#include "bls.h"
#include <stdio.h>

int hex2bin(u8 *code, u8 *c);
void skipblanks(const char **l);
int64_t parse_hex_skip(const char **cp);
int64_t parse_hex(const char *cp);
int64_t parse_int_skip(const char **cp);
int64_t parse_int(const char *cp);
void parse_sym(char *s, const char **cp);
size_t getsymname(char *output, const char *path);
void hexdump(const u8 *data, int size, u32 offset);

int hex2bin(u8 *code, u8 *c)
{
  int bc = 0;
  int b = 0;
  u8 val = 0;

  while(*c) {
    if(*c >= 'A' && *c <= 'F') {
      val <<= 4;
      val |= *c + 10 - 'A';
      ++b;
    } else if(*c >= 'a' && *c <= 'f') {
      val <<= 4;
      val |= *c + 10 - 'a';
      ++b;
    } else if(*c >= '0' && *c <= '9') {
      val <<= 4;
      val |= *c - '0';
      ++b;
    }

    ++c;

    if(b == 2) {
      b = 0;
      ++bc;
      *(code++) = (u8)val;
    }
  }

  return bc;
}

void skipblanks(const char **l)
{
  while(**l && **l <= ' ') {
    ++*l;
  }
}

int64_t parse_hex_skip(const char **cp)
{
  skipblanks(cp);
  int64_t val = 0;
  int len = 8;

  while(**cp && len--) {
    if(**cp >= 'A' && **cp <= 'F') {
      val <<= 4;
      val |= **cp + 10 - 'A';
    } else if(**cp >= 'a' && **cp <= 'f') {
      val <<= 4;
      val |= **cp + 10 - 'a';
    } else if(**cp >= '0' && **cp <= '9') {
      val <<= 4;
      val |= **cp - '0';
    } else {
      break;
    }

    ++(*cp);
  }

  return val;
}

int64_t parse_hex(const char *cp)
{
  return parse_hex_skip(&cp);
}

int64_t parse_int_skip(const char **cp)
{
  skipblanks(cp);
  int64_t val = 0;
  int neg = 0;

  if(**cp == '-') {
    ++(*cp);
    neg = 1;
  } else if(**cp == '~') {
    ++(*cp);
    neg = 2;
  }

  if(**cp == '$') {
    ++(*cp);
    val = parse_hex_skip(cp);
  } else if(**cp == '0' && (cp[0][1] == 'x' || cp[0][1] == 'X')) {
    (*cp) += 2;
    val = parse_hex_skip(cp);
  } else while(**cp) {
      if(**cp >= '0' && **cp <= '9') {
        val *= 10;
        val += **cp - '0';
      } else if(**cp == '*') {
        ++(*cp);
        int64_t second = parse_int_skip(cp);
        val *= second;
        break;
      } else {
        break;
      }

      ++(*cp);
    }

  if(neg == 1) {
    val = neint(val);
  } else if(neg == 2) {
    val = not_int(val);
  }

  return val;
}

int64_t parse_int(const char *cp)
{
  return parse_int_skip(&cp);
}

void parse_sym(char *s, const char **cp)
{
  const char *c = *cp;

  while((*c >= 'a' && *c <= 'z') || (*c >= 'A' && *c <= 'Z') || (*c >= '0' && *c <= '9') || *c == '_' || *c == '.') {
    *s = *c;
    ++s;
    ++c;
  }

  *s = '\0';
  *cp = c;
}

size_t getsymname(char *output, const char *path)
{
  int sep;
  char *op = output;

  // Parse first character

  if(!path || !*path) {
    printf("Error : Empty path\n");
    exit(1);
  }

  if(*path >= '0' && *path <= '9') {
    printf("Warning : symbols cannot start with numbers, prepending underscore to [%s]\n", path);
    *op = '_';
    ++op;
    sep = 0;
  }

  if(*path >= 'a' && *path <= 'z') {
    *op = *path - 'a' + 'A';
    ++op;
    ++path;
    sep = 0;
  } else if(*path >= 'A' && *path <= 'Z') {
    *op = *path;
    ++op;
    ++path;
    sep = 0;
  } else {
    *op = '_';
    ++op;
    ++path;
    sep = 1;
  }

  while(*path) {
    if((*path >= '0' && *path <= '9') || (*path >= 'A' && *path <= 'Z') || *path == '_') {
      *op = *path;
      ++op;
      sep = 0;
    } else if(*path >= 'a' && *path <= 'z') {
      *op = *path - 'a' + 'A';
      ++op;
      sep = 0;
    } else {
      if(!sep) {
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

void hexdump(const u8 *data, int size, u32 offset)
{
  int c;

  while(size) {
    // Print line
    printf("%06X: ", offset);
    const u8 *asciidata = data;
    int asciisize = 0;

    // Display hex dump
    for(c = 0; c < 16 && size; ++c, ++data, ++offset, --size) {
      printf("%02X ", *data);
      ++asciisize;
    }

    printf("  ");

    // Pad ASCII on the right
    for(c = asciisize; c < 16; ++c) {
      printf("   ");
    }

    // Display ASCII dump
    for(c = 0; c < asciisize; ++c) {
      if(asciidata[c] >= 0x20 && asciidata[c] < 0x7F) {
        printf("%c", asciidata[c]);
      } else {
        printf(".");
      }
    }

    printf("\n");
  }
}


// vim: ts=2 sw=2 sts=2 et
