#include "blsgen.h"
#include "mdconf.h"

mdconfnode_t *mdconf = 0;
BLSLL(source_t) *sources = 0;
BLSLL(section_t) *sections = 0;
BLSLL(output_t) *outputs = 0;
BLSLL(symbol_t) *symbols = 0;

void blsconf_load(const char *file) {
  mdconf = mdconfparsefile(file);

  mdconfnode_t *n;

  for(n = mdconf->child; (n = mdconfsearch(n, "source")); n = n->next) {
    sources = blsll_create_source_t(sources);
    source_t *s = sources->data;

    s->name = strdupnul(mdconfget(n->child, "name"));
    if(!s->name) s->name = strdupnul(n->value);
    s->format = format_parse(mdconfget(n->child, "format"));
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
        s->sections = blsll_create_section_t(NULL);
        s->sections->data->name = strdup("");
        s->sections->data->datafile = strdup("");
        parse_section(s->sections->data, n);
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

  for(n = mdconf->child; n = mdconfsearch(n, "output"); n = n->next) {
    outputs = blsll_create_output_t(outputs);
    output_t *o = outputs->data;

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

    o-> = strdupnul(mdconfget(n, ""));
  }
}

void parse_section(const section_t *s, const mdconfnode_t *md)
  const char *v;

  if((v = mdconfget(md, "physaddr")))
    s->physaddr = parse_int(v);
  else s->physaddr = -1;

  if((v = mdconfget(md, "physsize")))
    s->physsize = parse_int(v);
  else s->physsize = -1;

  if((v = mdconfget(md, "format")))
    s->format = format_parse(v);
  else s->format = format_raw;

  symbols = blsll_create_symbol_t(symbols);

  s->symbol = symbols->data;

  if((v = mdconfget(md, "chip")))
    s->symbol->value.chip = parse_chip(v);
  else s->symbol->value.chip = chip_none;

  if((v = mdconfget(md, "addr")))
    s->symbol.value.addr = parse_int(v);
  else s->addr.value = -1;

  if((v = mdconfget(md, "size")))
    s->size = parse_int(v);
  else s->size = -1;

}

char *symname(const char *s)
{
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
