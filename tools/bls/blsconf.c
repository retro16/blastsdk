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
GLOBLL(section, usedsections)
GLOBLL(group, binaries)
GLOBLL(group, usedbinaries)
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
  { ".s", format_gcc },
  { ".S", format_gcc },
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

char *strdupnorm(const char *s, int len)
{
  if(!s) {
    return NULL;
  }

  char *target = malloc(len + 1);
  target[len] = '\0';
  char *t = target;

  while(*s && len) {
    *t = *s;
    ++s;
    ++t;
    --len;
  }

  while(len) {
    *t = ' ';
    ++t;
    --len;
  }

  return target;
}

format format_guess(const char *name)
{
  // Try to deduce format from file/section name
  const char *c = strchr(name, '.');

  if(c) {
    int i = 0;

    while(extfmt[i].fmt != format_max) {
      if(strcasecmp(c, extfmt[i].ext) == 0) {
        return extfmt[i].fmt;
      }

      ++i;
    }
  }

  return format_auto;
}

group *source_parse(const mdconfnode *n, const char *name)
{
  group *s = source_find(name);

  if(!s) {
    sources = blsll_insert_group(sources, group_new());
    s = sources->data;
  } else if(!n) {
    return s;
  }

  bankreset(&s->banks);
  s->name = strdupnul(name);
  s->format = format_parse(mdconfget(n, "format"));

  if(s->format == format_auto && s->name) {
    s->format = format_guess(s->name);
  }

  MDCONF_GET_ENUM(n, bus, bus, s->banks.bus);
  const mdconfnode *banknode;

  for(banknode = n; (banknode = mdconfsearch(banknode, "bank")); banknode = banknode->next) {
    const char *bank = strchr(banknode->value, ':');

    if(!bank) {
      continue;
    }

    char chipname[8];
    strncpy(chipname, banknode->value, bank - banknode->value);
    chipname[bank - banknode->value] = '\0';
    chip chip = chip_parse(chipname);

    if(chip != chip_none) {
      s->banks.bank[chip] = parse_int(bank);
    }
  }

  switch(s->format) {
  case format_auto:
    break;

  case format_empty:
  case format_zero:
    s->provides = blsll_insert_section(NULL, section_parse_ext(NULL, name, ""));
    s->provides->data->source = s;
    break;

  case format_raw:
//      section_create_raw(s, n);
    break;

  case format_asmx:
    section_create_asmx(s, n);
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
    section_create_png(s, n);
    break;

  case format_max:
    break;
  }

  return s;
}

section *section_parse_nosrc(const mdconfnode *md, const char *name);

void output_parse(const mdconfnode *n, const char *name)
{
  mainout.name = strdupnorm(name, 48);
  maintarget = target_parse(mdconfget(n, "target"));
  /*
  mainout.name = strdupnul(mdconfget(n, "name"));
  if(!mainout.name) mainout.name = strdupnul(n->value);
  */
  mainout.copyright = strdupnorm(mdconfget(n, "copyright"), 16);
  mainout.region = strdupnorm(mdconfget(n, "region"), 16);
  mainout.notes = strdupnorm(mdconfget(n, "notes"), 40);
  mainout.rom_end = -2;

  if(mdconfget(n, "pad")) {
    mainout.rom_end = -1;
  }

  mainout.file = strdupnul(mdconfget(n, "file"));

  if(!mainout.file) {
    char n[4096];
    strcpy(n, name);

    switch(maintarget) {
    case target_gen:
      strcat(n, ".bin");
      break;

    case target_scd1:
    case target_scd2:
      strcat(n, ".iso");
      break;

    case target_ram:
      strcat(n, ".ram");
      break;

    default:
      break;
    }

    mainout.file = strdupnul(n);
  }

  const mdconfnode *ni;

  for(ni = n; (ni = mdconfsearch(ni, "binaries")); ni = ni->next) {
    const char *name = ni->value;

    // Represents a source
    group *g = binary_parse(NULL, name);
    mainout.binaries = blsll_insert_group(mainout.binaries, g);
  }
}

void blsconf_load(const char *file)
{
  mdconf_prefixes = &path_prefixes;
  mdconf = mdconfparsefile(file);

  if(!mdconf) {
    printf("Could not load file %s\n", file);
    exit(1);
  }

  const mdconfnode *n;

  n = mdconfsearch(mdconf->child, "output");

  if(n) {
    output_parse(n->child, mdconfgetvalue(n, "name"));
  }

  for(n = mdconf->child; (n = mdconfsearch(n, "section")); n = n->next) {
    section_parse(n->child, NULL, mdconfgetvalue(n, "name"));
  }

  for(n = mdconf->child; (n = mdconfsearch(n, "source")); n = n->next) {
    source_parse(n->child, mdconfgetvalue(n, "name"));
  }

  for(n = mdconf->child; (n = mdconfsearch(n, "binary")); n = n->next) {
    binary_parse(n->child, mdconfgetvalue(n, "name"));
  }
}

symbol *symbol_parse(const mdconfnode *md, const char *name)
{
  symbol *s = symbol_find(name);

  if(!s) {
    symbols = blsll_insert_symbol(symbols, symbol_new());
    s = symbols->data;
    s->name = strdupnul(name);
  } else if(!md) {
    return s;
  }

  if(s->value.addr == -1) {
    MDCONF_GET_INT(md, addr, s->value.addr);
  }

  if(s->value.chip == chip_none) {
    MDCONF_GET_ENUM(md, chip, chip, s->value.chip);
  }

  return s;
}

section *section_parse_nosrc(const mdconfnode *md, const char *name)
{
  size_t slen = strlen(name);
  char *srcname = (char *)alloca(slen + 1);
  strcpy(srcname, name);
  char *lastdot = strrchr(srcname, '.');

  if(lastdot) {
    *lastdot = '\0';
  }

  return section_parse(md, srcname, name);
}

section *section_parse_ext(const mdconfnode *md, const char *srcname, const char *name)
{
  size_t slen = strlen(srcname) + strlen(name);
  char *sname = (char *)alloca(slen + 1);
  strcpy(sname, srcname);
  strcat(sname, name);
  return section_parse(md, srcname, sname);
}

section *section_parse(const mdconfnode *md, const char *srcname, const char *name)
{
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
  } else if(!md) {
    return s;
  }

  const mdconfnode *n;

  for(n = md; (n = mdconfsearch(n, "uses")); n = n->next) {
    const char *name = n->value;
    section *dep;

    if((dep = section_parse_nosrc(NULL, name))) {
      s->uses = blsll_insert_section(s->uses, dep);
    }
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

BLSLL(section) *section_list_merge(BLSLL(section) *dest, BLSLL(section) *src)
{
  BLSLL(section) *result = dest;
  section *d, *s;
  BLSLL(section) *dl;

  BLSLL_FOREACH(s, src) {
    d = NULL;
    dl = dest;
    BLSLL_FOREACH(d, dl) {
      if(strcmp(d->name, s->name) == 0) {
        break;
      }
    }

    if(!d) {
      result = blsll_insert_section(result, s);
    }
  }

  return result;
}

BLSLL(section) *section_list_add(BLSLL(section) *dest, section *s)
{
  BLSLL(section) *result = dest;
  section *d;
  BLSLL(section) *dl;

  d = NULL;
  dl = dest;
  BLSLL_FOREACH(d, dl) {
    if(strcmp(d->name, s->name) == 0) {
      break;
    }
  }

  if(!d) {
    result = blsll_insert_section(result, s);
  }

  return result;
}

group *binary_parse(const mdconfnode *mdnode, const char *name)
{
  group *bin = binary_find(name);

  if(!bin) {
    binaries = blsll_insert_group(binaries, group_new());
    bin = binaries->data;
    bin->name = strdupnul(name);
  } else if(!mdnode) {
    return bin;
  }

  const mdconfnode *n;

  int explicitdeps = 0;

  MDCONF_GET_ENUM(mdnode, bus, bus, bin->banks.bus);
  bankreset(&bin->banks);

  for(n = mdnode; (n = mdconfsearch(n, "bank")); n = n->next) {
    const char *bank = strchr(n->value, ':');

    if(!bank) {
      continue;
    }

    char chipname[8];
    strncpy(chipname, n->value, bank - n->value);
    chipname[bank - n->value] = '\0';
    chip chip = chip_parse(chipname);

    if(chip != chip_none) {
      bin->banks.bank[chip] = parse_int(bank);
    }
  }

  group *g;
  section *s;

  for(n = mdnode; (n = mdconfsearch(n, "provides")); n = n->next) {
    const char *name = n->value;
    char filename[4096];

    if(findfile(filename, name)) {
      // Matches an existing file : represents a source
      explicitdeps = 1;
      g = source_parse(n, name);

      if(bin->banks.bus != bus_none && g->banks.bus == bus_none) {
        g->banks = bin->banks;
      }

      printf("Binary provides source %s\n", g->name);
      bin->provides_sources = blsll_insert_group(bin->provides_sources, g);
      continue;
    } else if((s = section_parse_nosrc(NULL, name))) {
      // Represents a section name
      explicitdeps = 1;
      printf("Binary provides section %s\n", s->name);
      bin->provides = blsll_insert_section(bin->provides, s);
      continue;
    }
  }

  for(n = mdnode; (n = mdconfsearch(n, "source")); n = n->next) {
    explicitdeps = 1;
    const char *name = n->value;
    g = source_parse(NULL, name);

    if(bin->banks.bus != bus_none && g->banks.bus == bus_none) {
      g->banks = bin->banks;
    }

    printf("Binary provides explicit source %s\n", g->name);
    bin->provides_sources = blsll_insert_group(bin->provides_sources, g);
  }

  for(n = mdnode; (n = mdconfsearch(n, "uses")); n = n->next) {
    const char *name = n->value;

    if((g = binary_parse(NULL, name))) {
      // Represents a source
      bin->uses_binaries = blsll_insert_group(bin->uses_binaries, g);
      continue;
    }

    printf("Warning in binary %s: could not find dependency source or section %s\n", bin->name, name);
  }

  if(!explicitdeps && strchr(name, '.')) {
    char srcname[1024];

    // Binary not explicit : it may represent a source or a section
    if(findfile(srcname, name)) {
      // File exists as-is : binary is a source
      g = source_parse(NULL, name);

      if(bin->banks.bus != bus_none && g->banks.bus == bus_none) {
        g->banks = bin->banks;
      }

      bin->provides_sources = blsll_insert_group(bin->provides_sources, g);
    } else {
      // Try to remove the last extension which may be a section name
      strncpy(srcname, name, 1024);
      srcname[1023] = '\0';
      *strrchr(srcname, '.') = '\0';

      char realname[1024];

      if(findfile(realname, srcname)) {
        section *s = section_parse_nosrc(NULL, name);
        bin->provides = blsll_insert_section(bin->provides, s);
      }
    }
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

// vim: ts=2 sw=2 sts=2 et
