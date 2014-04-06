#include "blsgen.h"
#include "mdconf.h"
#include "blsconf.h"
#include "blsgen_ext.h"

mdconfnode *mdconf = 0;

// Global linked lists
#define GLOBLL(type, name) \
BLSLL(type) *name = 0; \
type * name ## _new() { \
  name = blsll_create_ ## type (name); \
  return name->data; \
}
GLOBLL(group, sources)
GLOBLL(section, sections)
GLOBLL(group, binaries)
GLOBLL(output, outputs)
GLOBLL(symbol, symbols)
#undef GLOBLL

const struct {
  const char *ext;
  format fmt;
} extfmt[] = {
  { ".bin", format_raw },
  { ".asm", format_asmx },
  { ".z80.c", format_sdcc },
  { ".c", format_gcc },
  { ".s", format_as },
  { ".S", format_as },
  { ".png", format_png },
  { ".zero", format_zero },
  { ".text", format_raw },
  { ".data", format_raw },
  { ".bss", format_empty },
  { ".image", format_raw },
  { ".palette", format_raw },
  { ".map", format_raw },

  { "", format_max}
};

format format_guess(const char *name) {
  // Try to deduce format from file/section name
  const char *c = strchr(name, '.');
  if(c) {
    int i;
    while(extfmt[i].fmt != format_max) {
      if(strcasecmp(c, extfmt[i].ext) == 0) return extfmt[i].fmt;
      ++i;
    }
  }

  return format_auto;
}

group * source_parse(const mdconfnode *n, const char *name) {
  group *s = source_find(name);
  if(!s)
  {
    sources = blsll_insert_group(sources, group_new());
    s = sources->data;
  }
  s->name = strdupnul(name);
  s->format = format_parse(mdconfget(n, "format"));
  if(s->format == format_auto && s->name) {
    s->format = format_guess(s->name);
  }
  
  switch(s->format) {
    case format_auto:
      break;
    case format_empty:
    case format_zero:
      s->provides = blsll_insert_section(NULL, section_parse_ext(n, name, ""));
      s->provides->data->source = s;
      break;
    case format_raw:
//      section_create_raw(s, n);
      break;
    case format_asmx:
//      section_create_asmx(s, n);
      break;
    case format_sdcc:
//      section_create_sdcc(s, n);
    case format_gcc:
      section_create_gcc(s, n);
      break;
    case format_as:
//      section_create_as(s, n);
      break;
    case format_png:
//      section_create_png(s, n);
      break;
    case format_max:
      break;
  }

  return s;
}

output * output_parse(const mdconfnode *n, const char *name)
{
  output *o = output_find(name);
  if(!o)
  {
    outputs = blsll_insert_output(outputs, output_new());
    o = outputs->data;
  }
  o->name = strdupnul(name);
  o->target = target_parse(mdconfget(n, "target"));
  /*
  o->name = strdupnul(mdconfget(n, "name"));
  if(!o->name) o->name = strdupnul(n->value);
  */
  o->region = strdupnul(mdconfget(n, "region"));
  o->file = strdupnul(mdconfget(n, "file"));
  if(!o->file) o->file = strdupnul(o->name);

  if(o->target == target_scd) {
    const BLSLL(section) *section = sections;
    BLSLL_FINDSTR(section, name, "ip.asm");
    if(section)
      o->ip = section->data;

    section = sections;
    BLSLL_FINDSTR(section, name, "sp.asm");
    if(section)
      o->sp = section->data;
  } else {
    const BLSLL(symbol) *entry = symbols;
    BLSLL_FINDSTR(entry, name, "MAIN");
    if(entry)
      o->entry = entry->data;
  }
  return o;
}

void blsconf_load(const char *file) {
  mdconf = mdconfparsefile(file);
  if(!mdconf) {
    printf("Could not load file %s\n", file);
  }

  const mdconfnode *n;

  for(n = mdconf->child; (n = mdconfsearch(n, "source")); n = n->next) {
    source_parse(n->child, mdconfgetvalue(n, "name"));
  }

  for(n = mdconf->child; (n = mdconfsearch(n, "section")); n = n->next) {
    section_parse(n->child, NULL, mdconfgetvalue(n, "name"));
  }

  for(n = mdconf->child; (n = mdconfsearch(n, "binary")); n = n->next) {
    binary_parse(n->child, mdconfgetvalue(n, "name"));
  }

  for(n = mdconf->child; (n = mdconfsearch(n, "output")); n = n->next) {
    output_parse(n->child, mdconfgetvalue(n, "name"));
  }
}

symbol * symbol_parse(const mdconfnode *md, const char *name) {
  symbol *s = symbol_find(name);
  if(!s) {
    symbols = blsll_insert_symbol(symbols, symbol_new());
    s = symbols->data;
    s->name = strdupnul(name);
  }
  MDCONF_GET_INT(md, addr, s->value.addr);
  MDCONF_GET_ENUM(md, chip, chip, s->value.chip);

  return s;
}

section * section_parse_ext(const mdconfnode *md, const char *srcname, const char *name) {
  size_t slen = strlen(srcname) + strlen(name);
  char * sname = (char *)alloca(slen + 1);
  strcpy(sname, srcname);
  strcat(sname, name);
  return section_parse(md, srcname, sname);
}

section * section_parse(const mdconfnode *md, const char *srcname, const char *name) {
  section *s = section_find(name);
  if(!s) {
    sections = blsll_insert_section(sections, section_new());
    s = sections->data;
    s->name = strdupnul(name);

    if(!srcname) {
      const char *c = strrchr(name, '.');
      if(!c) {
        c = name + strlen(name);
      }
      srcname = (const char *)alloca(c - name + 1);
      memcpy((char *)srcname, name, c  - name);
      ((char *)srcname)[c - name] = '\0';
    }

    s->source = source_parse(NULL, srcname);
    s->datafile = symname(name);
  }

  MDCONF_GET_INT(md, physaddr, s->physaddr);
  MDCONF_GET_INT(md, physsize, s->physsize);
  MDCONF_GET_ENUM(md, format, format, s->format);
  MDCONF_GET_INT(md, size, s->size);
  MDCONF_GET_STR(md, datafile, s->datafile);

  char *sname;
  if(s->symbol) {
    sname = strdup(s->symbol->name);
  } else {
    sname = symname(name);
  }
  s->symbol = symbol_parse(md, sname);
  free(sname);

  return s;
}

group * binary_parse(const mdconfnode *mdnode, const char *name) {
  group *bin = binary_find(name);
  if(!bin) {
    binaries = blsll_insert_group(binaries, group_new());
    bin = binaries->data;
    bin->name = strdupnul(name);
  }

  const mdconfnode *n;

  group *g;
  section *s;
  for(n = mdnode; (n = mdconfsearch(n, "provides")); n = n->next) {
    const char *name = n->value;
    if((s = section_find(name))) {
      // Represents a section name
      bin->provides = blsll_insert_section(bin->provides, s);
      continue;
    }

    // Represents a source
    g = source_parse(NULL, name);
    bin->provides = blsll_copy_section(g->provides, bin->provides);
  }

  for(n = mdnode; (n = mdconfsearch(n, "uses")); n = n->next) {
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

  return bin;
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
    while(*s && ((*s >= '0' && *s <= '9') || (*s >= 'a' && *s <= 'z') || (*s >= 'A' && *s <= 'Z'))) {
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
    while(*s && !((*s >= '0' && *s <= '9') || (*s >= 'a' && *s <= 'z') || (*s >= 'A' && *s <= 'Z'))) {
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
    while(*s && ((*s >= '0' && *s <= '9') || (*s >= 'a' && *s <= 'z') || (*s >= 'A' && *s <= 'Z'))) {
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
    while(*s && !((*s >= '0' && *s <= '9') || (*s >= 'a' && *s <= 'z') || (*s >= 'A' && *s <= 'Z'))) {
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
