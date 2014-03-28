#include "blsgen.h"
#include "mdconf.h"

mdconfnode *mdconf = 0;

// Global linked lists
#define GLOBLL(type, name) \
BLSLL(type) *name = 0; \
type * name ## _new() { \
  name = blsll_create_ ## type (name); \
  return name ## s->data; \
}
GLOBLL(group, sources)
GLOBLL(section, sections)
GLOBLL(group, binaries)
GLOBLL(output, outputs)
GLOBLL(symbol, symbols)
#undef GLOBLL

void source_parse(group *s, const mdconfnode *n, const char *name) {
  s->name = strdupnul(name);
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
      s->provides = blsll_insert_section(NULL, section_new());
      s->provides->data->name = strdup("");
      s->provides->data->datafile = strdup("");
      s->provides->data->source = s;
      section_parse(s->provides->data, n->child);
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

void output_parse(output *o, const mdconfnode *n, const char *name)
{
  o->target = target_parse(mdconfsearch(n, "target"));
  o->name = strdupnul(mdconfget(n, "name"));
  if(!o->name) o->name = strdupnul(n->value);
  o->region = strdupnul(mdconfget(n, "region"));
  o->file = strdupnul(mdconfget(n, "file"));
  if(!o->file) o->file = strdupnul(o->name);

  if(o->target == target_scd) {
    const BLSLL(section) *section = sections;
    BLSLL_FINDSTR(section, name, "ip.asm");
    o->ip = section->data;

    section = sections;
    BLSLL_FINDSTR(section, name, "sp.asm");
    o->sp = section->data;
  } else {
    const symbol *entry = symbols;
    BLSLL_FINDSTR(entry, name, "MAIN");
    o->entry = entry;
  }
}

void blsconf_load(const char *file) {
  mdconf = mdconfparsefile(file);

  mdconfnode *n;
  for(n = mdconf->child; (n = mdconfsearch(n, "source")); n = n->next) {
    sources = blsll_insert_group(sources, group_new());
    source_parse(sources->data, n->child, mdconfgetvalue(n, "name"));
  }

  for(n = mdconf->child; n = mdconfsearch(n, "output"); n = n->next) {
    outputs = blsll_insert_output(outputs, output_new());
    output_parse(outputs->data, n->child, mdconfgetvalue(n, "name"));
  }
}

void section_parse(const section *s, const mdconfnode *md)
  MDCONF_GET_INT(md, physaddr, s->physaddr, -1);
  MDCONF_GET_INT(md, physsize, s->physsize, -1);
  MDCONF_GET_ENUM(md, format, format, s->format, format_raw);

  symbols = blsll_create_symbol(symbols);
  s->symbol = symbols->data;

  MDCONF_GET_ENUM(md, chip, chip, s->symbol->value.chip, chip_none);
  MDCONF_GET_INT(md, addr, s->symbol->value.addr, -1);
  MDCONF_GET_INT(md, size, s->size, -1);
}

void binary_parse(group *b, const mdconfnode *mdnode) {
  group *bin = group_new();
  bin->name = strdupnul(mdconfgetvalue(mdnode, "name"));
  binaries = blsll_insert_group(binaries, bin);

  group *g;
  section *s;
  for(n = mdnode->child; (n = mdconfsearch(n, "provides")); n = n->next) {
    const char *name = n->value;
    if((s = section_find(name))) {
      // Represents a section name
      bin->provides = blsll_insert_section(bin->provides, s);
      continue;
    }
    if((g = source_find(name))) {
      // Represents a source
      bin->provides = blsll_copy_section(g->provides, bin->provides);
      continue;
    }

    // Name not found : create a source with that name
    g = source_new();
    if(g->format != format_empty) {
      // Represents a newly created source
      sources = blsll_insert_group(sources, g);
      bin->provides = blsll_copy_section(g->provides, bin->provides);
      continue;
    } else {
      source_free(g);
    }

    printf("Warning in binary %s: could not find source or section %s\n", bin->name, name);
  }

  for(n = mdnode->child; (n = mdconfsearch(n, "uses")); n = n->next) {
    const char *name = n->value;
    if((s = section_find(name))) {
      // Represents a section name
      bin->uses = blsll_insert_section(bin->uses, s);
      continue;
    }
    if((g = source_find(name))) {
      // Represents a source
      bin->uses = blsll_copy_section(g->uses, bin->uses);
      continue;
    }

    printf("Warning in binary %s: could not find dependency source or section %s\n", bin->name, name);
  }
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
