#include "blsll.h"
#include "blsgen.h"
#include "blsconf.h"
#include "blsaddress.h"
#include "blsgen_ext.h"

output mainout;

void skipblanks(const char **l)
{
  while(**l && **l <= ' ')
  {
    ++*l;
  }
}

sv parse_hex_skip(const char **cp)
{
  skipblanks(cp);
  sv val = 0;
  int len = 8;
  while(**cp && len--)
  {
    if(**cp >= 'A' && **cp <= 'F')
    {
      val <<= 4;
      val |= **cp + 10 - 'A';
    }
    else if(**cp >= 'a' && **cp <= 'f')
    {
      val <<= 4;
      val |= **cp + 10 - 'a';
    }
    else if(**cp >= '0' && **cp <= '9')
    {
      val <<= 4;
      val |= **cp - '0';
    }
    else
    {
      break;
    }
    ++(*cp);
  }
  return val;
}

sv parse_hex(const char *cp) {
  return parse_hex_skip(&cp);
}

sv parse_int_skip(const char **cp) {
  skipblanks(cp);
  sv val = 0;
  int neg = 0;
  if(**cp == '-')
  {
    ++(*cp);
    neg = 1;
  } else if(**cp == '~')
  {
    ++(*cp);
    neg = 2;
  }
  if(**cp == '$')
  {
    ++(*cp);
    val = parse_hex_skip(cp);
  }
  else if(**cp == '0' && (cp[0][1] == 'x' || cp[0][1] == 'X'))
  {
    (*cp) += 2; 
    val = parse_hex_skip(cp);
  }
  else while(**cp)
  {
    if(**cp >= '0' && **cp <= '9')
    {
      val *= 10;
      val += **cp - '0';
    }
    else if(**cp == '*')
    {
      ++(*cp);
      sv second = parse_int_skip(cp);
      val *= second;
      break;
    }
    else
    {
      break;
    }
    ++(*cp);
  }
  if(neg == 1) {
    val = neg_int(val);
  } else if(neg == 2) {
    val = not_int(val);
  }
  return val;
}

sv parse_int(const char *cp) {
  return parse_int_skip(&cp);
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

group * group_new() {
  group *g = (group *)calloc(1, sizeof(group));
  g->optimize = -1;
  return g;
}

void group_free(group *p) {
  if(p->name) free(p->name);
  free(p);
}

const char bus_names[][8] = {"none", "main", "sub", "z80"};
const char chip_names[][8] = {"none", "mstack", "sstack", "zstack", "cart", "bram", "zram", "vram", "cram", "ram", "pram", "wram", "pcm"};
const char format_names[][8] = {"auto", "empty", "zero", "raw", "asmx", "sdcc", "gcc", "as", "png"};
const char target_names[][8] = {"gen", "scd", "vcart"};

section * section_new() {
  section *sec = (section *)calloc(1, sizeof(section));
  sec->physaddr = -1;
  sec->physsize = -1;
  sec->size = -1;
  return sec;
}

void section_free(section *p) {
  if(p->name) free(p->name);
  if(p->datafile) free(p->datafile);
  free(p);
}

symbol * symbol_new() {
  symbol *sym = (symbol *)calloc(1, sizeof(symbol));
  sym->value.addr = -1;
  return sym;
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

group * grouplist_find_sym(BLSLL(group) * groups, const char *name) {
  char symname[4096];
  getsymname(symname, name);

  group *g;
  char sn[4096];

  BLSLL_FOREACH(g, groups) {
    getsymname(sn, g->name);
    if(strcmp(symname, sn) == 0) {
      return g;
    }
  }

  return NULL;
}

section * sectionlist_find_sym(BLSLL(section) * sections, const char *name) {
  char symname[4096];
  getsymname(symname, name);

  section *g;
  char sn[4096];

  BLSLL_FOREACH(g, sections) {
    getsymname(sn, g->name);
    if(strcmp(symname, sn) == 0) {
      return g;
    }
  }

  return NULL;
}

group * source_find_sym(const char *name) {
  return grouplist_find_sym(sources, name);
}

section * section_find(const char *name) {
  BLSLL(section) *n = sections;
  BLSLL_FINDSTR(n, name, name);
  if(n)
    return n->data;
  return NULL;
}

section * section_find_sym(const char *name) {
  return sectionlist_find_sym(sections, name);
}

section * section_find_ext(const char *name, const char *suffix) {
  char *n = alloca(strlen(name) + strlen(suffix) + 1);
  strcpy(n, name);
  strcat(n, suffix);
  return section_find(n);
}

symbol * symbol_find(const char *name) {
  BLSLL(symbol) *n = symbols;
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

group * binary_find_sym(const char *name) {
  return grouplist_find_sym(binaries, name);
}

group * binary_find_providing(BLSLL(group) * glist, section *sec) {
  group *g;
  BLSLL_FOREACH(g, glist) {
    BLSLL(section) *sl = g->provides;
		section *s;
    BLSLL_FOREACH(s, sl) {
      if(sec == s) {
        return g;
      }
    }
  }

  return NULL;
}

int sections_overlap(section *s1, section *s2) {
  if(s1->symbol->value.chip == chip_none || s1->symbol->value.addr == -1 || s1->size == -1) {
    printf("Error : undefined address or size for section %s\n", s1->name);
    exit(1);
  }

  if(s2->symbol->value.chip == chip_none || s2->symbol->value.addr == -1 || s2->size == -1) {
    printf("Error : undefined address or size for section %s\n", s2->name);
    exit(1);
  }

  if(s1->symbol->value.chip != s2->symbol->value.chip) {
    // Different chips (or no chip)
    return 0;
  }

  if(s1->symbol->value.chip != chip_cart) {
    // Find if sections have binary-level dependencies
    // cart is ignored because cartridges have fixed content
    group *b1 = binary_find_providing(binaries, s1);
    if(b1) {
      group *b2 = binary_find_providing(binaries, s2);
      if(b2) {
        BLSLL(group) *bl = b2->uses_binaries;
        BLSLL_FOREACH(b2, bl) {
          if(b1 == b2) goto sections_overlap_same_binaries;
        }
      }
      // Sections are in binaries without dependencies : they cannot overlap
      return 0;
    }
  }
sections_overlap_same_binaries:
  while(0);

  sv s1addr = s1->symbol->value.addr;
  sv s1end = s1addr + s1->size;
  sv s2addr = s2->symbol->value.addr;
  sv s2end = s2addr + s2->size;

  if(s1addr >= s2end || s2addr >= s1end)
  {
    return 0;
  }

  return 1;
}

int sections_phys_overlap(section *s1, section *s2) {
  if(s1->physaddr == -1 || s1->physsize == -1) {
    printf("Error : undefined pĥysical location for section %s\n", s1->name);
    exit(1);
  }

  if(s2->physaddr == -1 || s2->physsize == -1) {
    printf("Error : undefined pĥysical location for section %s\n", s2->name);
    exit(1);
  }

  sv s1addr = s1->physaddr;
  sv s1end = s1addr + s1->physsize;
  sv s2addr = s2->physaddr;
  sv s2end = s2addr + s2->physsize;

  if(s1addr >= s2end || s2addr >= s1end)
  {
    return 0;
  }

  return 1;
}

symbol * symbol_set(BLSLL(symbol) **symlist, char *symname, chipaddr value, section *section) {
  BLSLL(symbol) *sl = *symlist;
  symbol *s;
  BLSLL_FOREACH(s, sl) {
    if(strcmp(symname, s->name) == 0) {
      break;
    }
  }
  if(!sl) {
    // Symbol not found
    s = symbol_find(symname);
    if(!s)
    {
      if(symlist)
      {
        *symlist = blsll_create_symbol(*symlist);
        s = (*symlist)->data;
      } else {
        s = (symbol*)calloc(1, sizeof(symbol));
      }
      s->name = strdup(symname);
      s->value.chip = chip_none;
      s->value.addr = -1;
      s->section = section;
      symbols = blsll_insert_symbol(symbols, s);
    }
    else
    {
      if(symlist) {
        *symlist = blsll_insert_symbol(*symlist, s);
      }
    }
  }

  if(!s->section) {
    // The symbol section was undefined
    s->section = section;
  } else if(section && section != s->section) {
    printf("Warning : symbol %s multiply defined (%s and %s at least).\n", symname, section->name, s->section->name);
  }

  if(value.chip != chip_none && value.addr != -1) {
    s->value = value;
  }

  return s;
}

symbol * symbol_set_bus(BLSLL(symbol) **symlist, char *symname, busaddr value, section *section) {
  chipaddr ca = bus2chip(value);
  return symbol_set(symlist, symname, ca, section);
}

void *merge_lists(void *target, void *source) {
  BLSLL(section) *rl = (BLSLL(section) *)target;
  BLSLL(section) *sl = (BLSLL(section) *)source;
  section *t, *s;

  BLSLL_FOREACH(s, sl) {
    BLSLL(section) *tl = rl;
    BLSLL_FOREACH(t, tl) {
      if(t == s) break;
    }
    if(!tl) {
      rl = blsll_insert_section(rl, s);
    }
  }

  return rl;
}

void group_dump(const group *grp, FILE *out) {
  BLSLL(section) *secl;
  section *sec;

  fprintf(out, " - format %s\n", format_names[grp->format]);

  if(grp->optimize >= 0) {
    fprintf(out, " - optimize %d\n", grp->optimize);
  }

  fprintf(out, " - bus %s\n - bank %d\n", bus_names[grp->bus], grp->banks.bank[grp->bus]);

  if(grp->provides) {
    secl = grp->provides;

    fprintf(out, " - provides ");
    BLSLL_FOREACH(sec, secl) {
      fprintf(out, "`%s`", sec->name);
      if(secl->next) {
        fprintf(out, ", ");
      }
    }
    fprintf(out, "\n");
  }

  if(grp->provides_sources) {
    BLSLL(group) *secl = grp->provides_sources;
    group *sec;

    fprintf(out, " - provides ");
    BLSLL_FOREACH(sec, secl) {
      fprintf(out, "`%s`", sec->name);
      if(secl->next) {
        fprintf(out, ", ");
      }
    }
    fprintf(out, " (source)\n");
  }

  if(grp->uses_binaries) {
    BLSLL(group) *secl = grp->uses_binaries;
    group *sec;

    fprintf(out, " - uses ");
    BLSLL_FOREACH(sec, secl) {
      fprintf(out, "`%s`", sec->name);
      if(secl->next) {
        fprintf(out, ", ");
      }
    }
    fprintf(out, " (binary)\n");
  }

  if(grp->loads) {
    BLSLL(group) *secl = grp->loads;
    group *sec;

    fprintf(out, " - loads ");
    BLSLL_FOREACH(sec, secl) {
      fprintf(out, "`%s`", sec->name);
      if(secl->next) {
        fprintf(out, ", ");
      }
    }
    fprintf(out, " (binary)\n");
  }
}

void symtable_dump(const BLSLL(symbol) *syml, FILE *out)
{
  const symbol *sym;
  BLSLL_FOREACH(sym, syml) {
    if(!sym->name) {
      fprintf(out, "Unnamed symbol\n");
      continue;
    }
    fprintf(out, "    %5s ", chip_names[sym->value.chip]);
    if(sym->value.addr >= 0) {
      fprintf(out, "0x%08X", (uint32_t)sym->value.addr);
    } else {
      fprintf(out, "??????????");
    }
    fprintf(out, " %s\n", sym->name);
  }
}

void section_dump(const section *sec, FILE *out)
{
    fprintf(out, " - datafile `%s`\n", sec->datafile);
    fprintf(out, " - format %s\n", format_names[sec->format]);

    if(sec->physaddr != -1) {
      fprintf(out, " - physaddr $%08lX\n", (uint64_t)sec->physaddr);
    }
    if(sec->physsize != -1) {
      fprintf(out, " - physsize $%08lX\n", (uint64_t)sec->physsize);
    }
    if(sec->physalign) {
      fprintf(out, " - physalign $%X\n", (unsigned int)sec->physalign);
    }

    if(sec->symbol) {
      fprintf(out, " - chip %s\n", chip_names[sec->symbol->value.chip]);
      if(sec->symbol->value.addr != -1) {
        fprintf(out, " - addr $%08lX", (uint64_t)sec->symbol->value.addr);
        if(sec->source && sec->source->bus != bus_none) {
          busaddr ba = chip2bus(sec->symbol->value, sec->source->bus);
          if(ba.addr != -1) {
            fprintf(out, " ($%08X bus=%s bank=%d)", (unsigned int)ba.addr, bus_names[sec->source->bus], ba.bank);
          }
        }
        fprintf(out, "\n");
      }
    }

    if(sec->align) {
      fprintf(out, " - align $%X\n", (unsigned int)sec->align);
    }
    if(sec->size >= 0) {
      fprintf(out, " - size $%lX\n", (uint64_t)sec->size);
    }

    if(sec->uses) {
      BLSLL(section) *secl = sec->uses;
      section *s;

      fprintf(out, " - uses ");
      BLSLL_FOREACH(s, secl) {
        fprintf(out, "`%s`", s->name);
        if(secl->next) {
          fprintf(out, ", ");
        }
      }
      fprintf(out, "\n");
    }

    if(sec->loads) {
      BLSLL(group) *grpl = sec->loads;
      group *grp;

      fprintf(out, " - loads ");
      BLSLL_FOREACH(grp, grpl) {
        fprintf(out, "`%s`", grp->name);
        if(grpl->next) {
          fprintf(out, ", ");
        }
      }
      fprintf(out, "\n");
    }

    fprintf(out, "\n");

    if(sec->symbol) {
      if(sec->symbol->name) {
        fprintf(out, "\nSymbol name `%s`\n\n", sec->symbol->name);
      }
    }

    if(sec->intsym) {
      fprintf(out, "\nInternal symbol table :\n\n");
      symtable_dump(sec->intsym, out);
    }

    if(sec->extsym) {
      fprintf(out, "\nExternal symbol table :\n\n");
      symtable_dump(sec->extsym, out);
    }

}

void output_dump(FILE *out) {
  fprintf(out, " - target %s\n", target_names[mainout.target]);
  if(mainout.region) {
    fprintf(out, " - region %s\n", mainout.region);
  }
  if(mainout.file) {
    fprintf(out, " - file `%s`\n", mainout.file);
  }
  if(mainout.target == target_scd) {
    if(mainout.ip) {
      fprintf(out, " - ip `%s`\n", mainout.ip->name);
    }
    if(mainout.sp) {
      fprintf(out, " - sp `%s`\n", mainout.sp->name);
    }
  } else if(mainout.entry) {
    fprintf(out, " - entry `%s`\n", mainout.entry->name);
  }

  const BLSLL(group) *grpl;
  group *grp;
  if(mainout.binaries) {
    fprintf(out, " - binaries ");
    grpl = mainout.binaries;
    BLSLL_FOREACH(grp, grpl) {
      fprintf(out, "`%s`", grp->name);
      if(grpl->next) {
        fprintf(out, ", ");
      }
    }
    fprintf(out, "\n");
  }

  if(mainout.bol) {
    fprintf(out, "\nBuild order list: ");
    grpl = mainout.bol;
    BLSLL_FOREACH(grp, grpl) {
      fprintf(out, "`%s`", grp->name);
      if(grpl->next) {
        fprintf(out, ", ");
      }
    }
    fprintf(out, "\n");
  }
}

void blsconf_dump(FILE *out) {
  // Dumps the whole configuration to a FILE

  fprintf(out, "blsgen configuration dump\n");

  BLSLL(section) *secl;
  section *sec;
  BLSLL(group) *grpl;
  group *grp;

  // Dump sources
  fprintf(out, "\n----------------------------------------\n");
  fprintf(out, "\nSources\n=======\n\n");
  grpl = sources;
  BLSLL_FOREACH(grp, grpl) {
    fprintf(out, "Source `%s`\n------\n\n", grp->name);
    group_dump(grp, out);
    fprintf(out, "\n");
  }

  // Dump sections
  fprintf(out, "\n----------------------------------------\n");
  fprintf(out, "\nSections\n========\n\n");
  secl = sections;
  BLSLL_FOREACH(sec, secl) {
    fprintf(out, "Section `%s`\n-------\n\n", sec->name);
    section_dump(sec, out);
    fprintf(out, "\n");
  }

  // Dump binaries
  fprintf(out, "\n----------------------------------------\n");
  fprintf(out, "\nBinaries\n========\n\n");
  grpl = binaries;
  BLSLL_FOREACH(grp, grpl) {
    fprintf(out, "Binary `%s`\n------\n\n", grp->name);
    group_dump(grp, out);
    fprintf(out, "\n");
  }

  // Dump output
  fprintf(out, "\n----------------------------------------\n");
  fprintf(out, "\nOutput\n=======\n\n");
  output_dump(out);
  fprintf(out, "\n");
}

void gen_bol(group *s)
{
  BLSLL(section) *secl;
  section *sec;
  BLSLL(group) *grpl;
  group *grp;
  BLSLL(symbol) *syml;
  symbol *sym;

  // Check if the source is already in the BOL
  grpl = mainout.bol;
  BLSLL_FOREACH(grp, grpl) {
    if(grp == s) {
      // Source already in the BOL : stop processing it.
      return;
    }
  }

  mainout.bol = blsll_append_group(mainout.bol, s);

  // For each section provided by the source
  secl = s->provides;
  BLSLL_FOREACH(sec, secl) {
    // For each external symbol
    syml = sec->extsym;
    BLSLL_FOREACH(sym, syml) {
      // Recurse with the source of the external symbol
      if(sym->section) {
        if(sym->section->source) {
          gen_bol(sym->section->source);
        }
        // Add this section to the section dependencies
        sec->uses = blsll_insert_unique_section(sec->uses, sym->section);
      }
    }
  }
}

void bls_gen_bol()
{
  if(mainout.target == target_scd) {
    gen_bol(mainout.ip->source);
    gen_bol(mainout.sp->source);
  } else {
    gen_bol(mainout.entry->section->source);
  }
}

void bls_find_entry()
{
  switch(mainout.target)
  {
    default:
      mainout.entry = symbol_find("MAIN");
      if(!mainout.entry) mainout.entry = symbol_find("MAIN_ASM");
      if(!mainout.entry) mainout.entry = symbol_find("main");
      if(!mainout.entry) {
        printf("Could not find MAIN entry point.\n");
        exit(1);
      }
      break;

    case target_scd:
      mainout.ip = section_find("IP_ASM");
      if(!mainout.ip) mainout.ip = section_find("IP_MAIN");
      if(!mainout.ip) mainout.ip = section_find("ip_main");
      if(!mainout.ip) {
        printf("Could not find IP entry point.\n");
        exit(1);
      }
      mainout.sp = section_find("SP_ASM");
      if(!mainout.sp) mainout.sp = section_find("SP_MAIN");
      if(!mainout.sp) mainout.sp = section_find("sp_main");
      if(!mainout.sp) {
        printf("Could not find SP entry point.\n");
        exit(1);
      }
      break;
  }
}

void bls_get_symbols()
{
  BLSLL(group) *grpl;
  group *grp;

  grpl = sources;
  BLSLL_FOREACH(grp, grpl) {
    switch(grp->format) {
      case format_gcc:
        source_get_symbols_gcc(grp);
        break;
      case format_png:
        source_get_symbols_png(grp);
        break;
      default:
        break;
    }
  }
}

void bls_get_symbol_values()
{
  BLSLL(group) *grpl;
  group *grp;

  grpl = sources;
  BLSLL_FOREACH(grp, grpl) {
    switch(grp->format) {
      case format_gcc:
        source_get_symbol_values_gcc(grp);
        break;
      default:
        break;
    }
  }
}

void bls_compile()
{
  BLSLL(group) *grpl;
  group *grp;

  grpl = sources;
  BLSLL_FOREACH(grp, grpl) {
    switch(grp->format) {
      case format_gcc:
        source_compile_gcc(grp);
        break;
      default:
        break;
    }
  }
}

void bls_map()
{
  BLSLL(group) *grpl;
  group *grp;

  // Premap all sources
  grpl = sources;
  BLSLL_FOREACH(grp, grpl) {
    switch(grp->format) {
      case format_gcc:
        printf("Premapping %s\n", grp->name);
        source_premap_gcc(grp);
        break;
      case format_png:
        printf("Premapping %s\n", grp->name);
        source_premap_png(grp);
        break;
      default:
        break;
    }
  }

  // Do logical section mapping
  BLSLL(section) *secl = sections;
  section *sec;
  BLSLL_FOREACH(sec, secl) {
    if(sec->size < 0) {
      printf("Error : section %s has an unknown size\n", sec->name);
      exit(1);
    } else if(!sec->size) {
      // Sections with null size don't need mapping
      continue;
    }
    if(sec->symbol->value.chip == chip_none) {
      printf("Error : could not determine chip of section %s\n", sec->name);
      exit(1);
    }
    if(sec->symbol->value.addr != -1) {
      // Section address already known
      continue;
    }

    // Find a place for this section
    sv chipstart = chip_start(sec->symbol->value.chip);
    sv chipend = chipstart + chip_size(sec->symbol->value.chip);
    sec->symbol->value.addr = chipstart;
    printf("Section %s : size=%04X chipstart=%06X chipend=%06X\n", sec->name, (unsigned int)sec->size, (unsigned int)chipstart, (unsigned int)chipend);
    while(sec->symbol->value.addr + sec->size <= chipend)
    {
      BLSLL(section) *sl = sections;
      section *s;
      BLSLL_FOREACH(s, sl) {
        if(s == sec || s->size == 0 || s->symbol->value.addr == -1) continue;
        printf("Overlaps ? %s <-> %s  ", sec->name, s->name);
        if(sections_overlap(sec, s)) {
          printf("YES : %06X => ", (unsigned int)sec->symbol->value.addr);
          sec->symbol->value.addr = s->symbol->value.addr + s->size;
          chip_align(&sec->symbol->value);
          printf("%06X\n", (unsigned int)sec->symbol->value.addr);
          goto bls_map_next_section_addr;
        } else {
          printf("NO\n");
        }
      }
      // Found
      goto bls_map_next_section;
bls_map_next_section_addr:
      continue;
    }

    if(sec->symbol->value.addr + sec->size >= chipend) {
      printf("Error : could not map section %s : not enough memory in chip %s\n", sec->name, chip_names[sec->symbol->value.chip]);
      exit(1);
    }
bls_map_next_section:
    continue;
  }

  // Do physical mapping

}

void bls_finalize_binary_dependencies()
{
  BLSLL(group) *grpl;
  group *grp;

  // Replace source dependencies with section dependencies
  grpl = binaries;
  BLSLL_FOREACH(grp, grpl) {
    if(grp->provides_sources) {
      BLSLL(group) *srcl = grp->provides_sources;
      group *src;
      BLSLL_FOREACH(src, srcl) {
        BLSLL(section) *sl = src->provides;
        section *s;

        BLSLL_FOREACH(s, sl) {
          grp->provides = blsll_insert_section(grp->provides, s);
        }
      }
      blsll_free_group(grp->provides_sources);
      grp->provides_sources = NULL;
    }
  }

  // Adds "loads" to binaries based on "loads" of its sections
  // Unifies data format
  grpl = binaries;
  BLSLL_FOREACH(grp, grpl) {
    if(grp->format == format_auto) {
      grp->format = format_raw;
    }
    BLSLL(section) *secl = grp->provides;
    section *sec;
    BLSLL_FOREACH(sec, secl) {
      if(sec->format == format_auto) {
        sec->format = grp->format;
      }
      grp->uses_binaries = (BLSLL(group) *)merge_lists(grp->uses_binaries, sec->loads);
      grp->loads = (BLSLL(group) *)merge_lists(grp->loads, sec->loads);
    }
  }

  // Find uses_binaries for each binary
  grpl = binaries;
  BLSLL_FOREACH(grp, grpl) {
    BLSLL(section) *secl = grp->provides;
    section *sec;
    BLSLL_FOREACH(sec, secl) {
      BLSLL(section) *usl = sec->uses;
      section *us;
      BLSLL_FOREACH(us, usl) {
        grp->uses_binaries = blsll_insert_unique_group(grp->uses_binaries, binary_find_providing(binaries, us));
      }
    }
  }

  // Add reverse dependencies to uses_binaries
  grpl = binaries;
  BLSLL_FOREACH(grp, grpl) {
    BLSLL(group) *ol = grp->uses_binaries;
    group *o;
    BLSLL_FOREACH(o, ol) {
      o->uses_binaries = blsll_insert_unique_group(o->uses_binaries, grp);
    }
  }
}

int main(int argc, char **argv) {
  blsconf_load(argc > 1 ? argv[1] : "blsgen.md");

  bls_get_symbols();
  bls_find_entry();
  bls_gen_bol();
  bls_finalize_binary_dependencies();
  bls_map();
  bls_get_symbol_values();
	bls_compile();

  blsconf_dump(stdout);
}
