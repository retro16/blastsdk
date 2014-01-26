#include "blsgen.h"
#include "mdconf.h"

mdconfnode_t *mdconf = 0;

// Global linked lists
#define GLOBLL(name) \
BLSLL(name ## _t) *name ## _s = 0; \
name ## _t * name ## _new() { \
  name ## s = blsll_create_ ## name ## _t(name ## s); \
  return name ## s->data; \
}
GLOBLL(source)
GLOBLL(section)
GLOBLL(output)
GLOBLL(symbol)
#undef GLOBLL

void source_parse(source_t *s, const mdconfnode_t *n) {
  s->name = strdupnul(mdconfgetvalue(n, "name"));
  n = n->child;
  s->format = format_parse(mdconfget(n, "format"));
  if(s->format == format_auto && s->name) {
    // Try to deduce format from file name
    const char *c = strchr(s->name, '.');
    if(c) {
      if(strcasecmp(c, ".bin") == 0) s->format = format_bin;
      else if(strcasecmp(c, ".asm") == 0) s->format = format_asmx;
      else if(strcasecmp(c, ".z80.c") == 0) s->format = format_sdcc;
      else if(strcasecmp(c, ".c") == 0) s->format = format_gcc;
      else if(strcasecmp(c, ".s") == 0) s->format = format_as;
      else if(strcasecmp(c, ".S") == 0) s->format = format_as;
      else if(strcasecmp(c, ".png") == 0) s->format = format_png;
      else s->format = format_empty;
    }
    else s->format = format_empty;
  }
  
  switch(s->format) {
    case format_empty:
      s->sections = blsll_insert_section_t(NULL, section_new());
      s->sections->data->name = strdup("");
      s->sections->data->datafile = strdup("");
      s->sections->data->source = s;
      section_parse(s->sections->data, n->child);
      break;
    case format_bin:
      section_create_bin(s, n);
      break;
    case format_asmx:
      section_create_asmx(s, n);
      break;
    case format_gcc:
      section_create_gcc(s, n);
      break;
    case format_as:
      section_create_as(s, n);
      break;
    case format_png:
      section_create_png(s, n);
      break;
  }
}

void output_parse(output_t *o, const mdconfnode *n, const char *name)
{
  o->target = target_parse(mdconfsearch(n, "target"));
  o->name = strdupnul(mdconfget(n, "name"));
  if(!o->name) o->name = strdupnul(n->value);
  o->region = strdupnul(mdconfget(n, "region"));
  o->file = strdupnul(mdconfget(n, "file"));
  if(!o->file) o->file = strdupnul(o->name);

  if(o->target == target_scd) {
    const BLSLL(section_t) *section = sections;
    BLSLL_FINDSTR(section, name, "ip.asm");
    o->ip = section->data;

    section = sections;
    BLSLL_FINDSTR(section, name, "sp.asm");
    o->sp = section->data;
  } else {
    const symbol_t *entry = symbols;
    BLSLL_FINDSTR(entry, name, "MAIN");
    o->entry = entry;
  }
}

void blsconf_load(const char *file) {
  mdconf = mdconfparsefile(file);

  mdconfnode_t *n;
  for(n = mdconf->child; (n = mdconfsearch(n, "source")); n = n->next) {
    source_parse(n->child, source_new(), mdconfgetvalue(n, "name"));
  }

  for(n = mdconf->child; n = mdconfsearch(n, "output"); n = n->next) {
    output_parse(n->child, output_new(), mdconfgetvalue(n, "name"));
  }
}

void section_parse(const section_t *s, const mdconfnode_t *md)
  MDCONF_GET_INT(md, physaddr, s->physaddr, -1);
  MDCONF_GET_INT(md, physsize, s->physsize, -1);
  MDCONF_GET_ENUM(md, format, format, s->format, format_raw);

  symbols = blsll_create_symbol_t(symbols);
  s->symbol = symbols->data;

  MDCONF_GET_ENUM(md, chip, chip, s->symbol->value.chip, chip_none);
  MDCONF_GET_INT(md, addr, s->symbol->value.addr, -1);
  MDCONF_GET_INT(md, size, s->size, -1);
}

char *symname(const char *s)
{
  if(!s) {
    return NULL;
  }
  size_t slen = strlen(s);
  char *sname = (char *)malloc(slen + 1);
  char *sn = sname;
  while(*s) {
    while(*s && (s >= '0' && s <= '9' || s >= 'a' && s <= 'z' || s >= 'A' && s <= 'Z')) {
      if(*s >= 'a') {
        *sn = *s + 'A' - 'a';
      } else {
        *sn = *s;
      }
      ++s; 
      ++sn;
    }

    if(*s) {
      ++s;
      *sn = '_';
      ++sn;
    }
    while(*s && !(s >= '0' && s <= '9' || s >= 'a' && s <= 'z' || s >= 'A' && s <= 'Z')) {
      ++s;
    }
  }
  *sn = '\0';
  return sname;
}

char *symname2(const char *s, const char *s2)
{
  size_t slen = strlen(s) + strlen(s2);
  char *sname = (char *)malloc(slen + 1);
  char *sn = sname;
  while(*s) {
    while(*s && (s >= '0' && s <= '9' || s >= 'a' && s <= 'z' || s >= 'A' && s <= 'Z')) {
      if(*s >= 'a') {
        *sn = *s + 'A' - 'a';
      } else {
        *sn = *s;
      }
      ++s; 
      ++sn;
    }

    if(*s) {
      ++s;
      *sn = '_';
      ++sn;
    }
    while(*s && !(s >= '0' && s <= '9' || s >= 'a' && s <= 'z' || s >= 'A' && s <= 'Z')) {
      ++s;
    }

    if(!*s && s2) {
      s = s2;
      s2 = NULL;
    }
  }
  *sn = '\0';
  return sname;
}
