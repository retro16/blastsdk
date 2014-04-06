#include "blsll.h"
#include "blsgen.h"
#include "blsconf.h"

void skipblanks(const char **l)
{
  while(**l && **l <= ' ')
  {
    ++*l;
  }
}

sv parse_hex(const char *cp)
{
  skipblanks(&cp);
  sv val = 0;
  int len = 8;
  while(*cp && len--)
  {
    if(*cp >= 'A' && *cp <= 'F')
    {
      val <<= 4;
      val |= *cp + 10 - 'A';
    }
    else if(*cp >= 'a' && *cp <= 'f')
    {
      val <<= 4;
      val |= *cp + 10 - 'a';
    }
    else if(*cp >= '0' && *cp <= '9')
    {
      val <<= 4;
      val |= *cp - '0';
    }
    else
    {
      break;
    }
    ++cp;
  }
  return val;
}

sv parse_int(const char *cp) {
  skipblanks(&cp);
  sv val = 0;
  int neg = 0;
  if(*cp == '-')
  {
    ++cp;
    neg = 1;
  } else if(*cp == '~')
  {
    ++cp;
    neg = 2;
  }
  if(*cp == '$')
  {
    ++cp;
    val = parse_hex(cp);
  }
  else if(*cp == '0' && (cp[1] == 'x' || cp[1] == 'X'))
  {
    cp += 2; 
    val = parse_hex(cp);
  }
  else while(*cp)
  {
    if(*cp >= '0' && *cp <= '9')
    {
      val *= 10;
      val += *cp - '0';
    }
    else if(*cp == 'x' || *cp == '*')
    {
      sv second = parse_int(cp + 1);
      val *= second;
      break;
    }
    else
    {
      break;
    }
    ++cp;
  }
  if(neg == 1) {
    val = neg_int(val);
  } else if(neg == 2) {
    val = not_int(val);
  }
  return val;
}


group * group_new() {
  return (group *)malloc(sizeof(group));
}

void group_free(group *p) {
  if(p->name) free(p->name);
  free(p);
}

const char bus_names[][8] = {"none", "main", "sub", "z80"};
const char chip_names[][8] = {"none", "stack", "cart", "bram", "zram", "vram", "ram", "pram", "wram", "pcm"};
const char format_names[][8] = {"auto", "empty", "zero", "raw", "asmx", "sdcc", "gcc", "as", "png"};

section * section_new() {
  return (section *)calloc(1, sizeof(section));
}

void section_free(section *p) {
  if(p->name) free(p->name);
  if(p->datafile) free(p->datafile);
  free(p);
}

symbol * symbol_new() {
  return (symbol *)calloc(1, sizeof(symbol));
}

void symbol_free(symbol *p) {
  if(p->name) free(p->name);
  free(p);
}

const char target_name[][8] = {"gen", "scd", "vcart"};

output * output_new() {
  return (output *)calloc(1, sizeof(output));
}

void output_free(output *p) {
  if(p->name) free(p->name);
  if(p->region) free(p->region);
  if(p->file) free(p->file);
  free(p);
}

group * source_find(const char *name) {
  BLSLL(group) *n = sources;
  BLSLL_FINDSTR(n, name, name);
  if(n)
    return n->data;
  return NULL;
}

section * section_find(const char *name) {
  BLSLL(section) *n = sections;
  BLSLL_FINDSTR(n, name, name);
  if(n)
    return n->data;
  return NULL;
}

symbol * symbol_find(const char *name) {
  BLSLL(symbol) *n = symbols;
  BLSLL_FINDSTR(n, name, name);
  if(n)
    return n->data;
  return NULL;
}

output * output_find(const char *name) {
  BLSLL(output) *n = outputs;
  BLSLL_FINDSTR(n, name, name);
  if(n)
    return n->data;
  return NULL;
}

group * binary_find(const char *name) {
  BLSLL(group) *n = binaries;
  BLSLL_FINDSTR(n, name, name);
  if(n)
    return n->data;
  return NULL;
}

int main() {
  blsconf_load("blsgen.md");

}
