#include "blsll.h"
#include "blsgen.h"
#include "blsconf.h"
#include "blsaddress.h"
#include "blsgen_ext.h"
#include "blspack.h"
#include "blswrite.h"
#include "blsparse.h"
#include "blsiso.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <libgen.h>

#include "scdboot_us.c"
#include "scdboot_eu.c"
#include "scdboot_jp.c"

output mainout;


group *group_new()
{
  group *g = (group *)calloc(1, sizeof(group));
  g->optimize = -1;
  g->physaddr = -1;
  g->physsize = -1;
  return g;
}

void group_free(group *p)
{
  if(p->name) {
    free(p->name);
  }

  if(p->file) {
    free(p->file);
  }

  free(p);
}

const char format_names[][8] = {"auto", "empty", "zero", "raw", "asmx", "sdcc", "gcc", "as", "png"};

section *section_new()
{
  section *sec = (section *)calloc(1, sizeof(section));
  sec->physaddr = -1;
  sec->physsize = -1;
  sec->size = -1;
  return sec;
}

void section_free(section *p)
{
  if(p->name) {
    free(p->name);
  }

  if(p->datafile) {
    free(p->datafile);
  }

  free(p);
}

symbol *symbol_new()
{
  symbol *sym = (symbol *)calloc(1, sizeof(symbol));
  sym->value.addr = -1;
  return sym;
}

void symbol_free(symbol *p)
{
  if(p->name) {
    free(p->name);
  }

  free(p);
}

output *output_new()
{
  return (output *)calloc(1, sizeof(output));
}

void output_free(output *p)
{
  if(p->name) {
    free(p->name);
  }

  if(p->region) {
    free(p->region);
  }

  if(p->file) {
    free(p->file);
  }

  free(p);
}

group *source_find(const char *name)
{
  BLSLL(group) *n = sources;
  BLSLL_FINDSTR(n, name, name);

  if(n) {
    return n->data;
  }

  return NULL;
}

group *grouplist_find_sym(BLSLL(group) * groups, const char *name)
{
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

section *sectionlist_find_sym(BLSLL(section) * sections, const char *name)
{
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

group *source_find_sym(const char *name)
{
  return grouplist_find_sym(sources, name);
}

section *section_find(const char *name)
{
  BLSLL(section) *n = sections;
  BLSLL_FINDSTR(n, name, name);

  if(n) {
    return n->data;
  }

  return NULL;
}

section *section_find_sym(const char *name)
{
  return sectionlist_find_sym(sections, name);
}

section *section_find_ext(const char *name, const char *suffix)
{
  char *n = alloca(strlen(name) + strlen(suffix) + 1);
  strcpy(n, name);
  strcat(n, suffix);
  return section_find(n);
}

symbol *symbol_find(const char *name)
{
  BLSLL(symbol) *n = symbols;
  BLSLL_FINDSTR(n, name, name);

  if(n) {
    return n->data;
  }

  return NULL;
}

sv symbol_get_addr(const char *name, bankconfig *bankconfig)
{
  symbol *sym = symbol_find(name);

  if(!sym) {
    return -1;
  }

  return chip2bank(sym->value, bankconfig);
}

chip symbol_get_chip(const char *name)
{
  symbol *sym = symbol_find(name);

  if(!sym) {
    return chip_none;
  }

  return sym->value.chip;
}

chipaddr symbol_get_chipaddr(const char *name)
{
  chipaddr notfound = {chip_none, -1};

  symbol *sym = symbol_find(name);

  if(!sym) {
    return notfound;
  }

  return sym->value;
}

sv symbol_get_bus(const char *name, bus bus)
{
  symbol *sym = symbol_find(name);

  if(!sym) {
    return -1;
  }

  busaddr ba = chip2bus(sym->value, bus);
  return ba.addr;
}

group *binary_find(const char *name)
{
  BLSLL(group) *n = binaries;
  BLSLL_FINDSTR(n, name, name);

  if(n) {
    return n->data;
  }

  return NULL;
}

group *binary_find_sym(const char *name)
{
  return grouplist_find_sym(binaries, name);
}

group *binary_find_providing(BLSLL(group) * glist, section *sec)
{
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

int sections_overlap(section *s1, section *s2)
{
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
    // TODO : support sections stored in multiple binaries
    group *b1 = binary_find_providing(binaries, s1);

    if(b1) {
      group *b2 = binary_find_providing(binaries, s2);

      if(b2) {
        if(b1 == b2) {
          goto sections_overlap_same_binaries;
        }

        BLSLL(group) *bl = b2->uses_binaries;
        BLSLL_FOREACH(b2, bl) {
          if(b1 == b2) {
            goto sections_overlap_same_binaries;
          }
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

  if(s1addr >= s2end || s2addr >= s1end) {
    return 0;
  }

  return 1;
}

int phys_overlap(element *s1, element *s2)
{
  if(s1->physaddr == -1 || s1->physsize == -1) {
    printf("Error : undefined physical location for section %s\n", s1->name);
    exit(1);
  }

  if(s2->physaddr == -1 || s2->physsize == -1) {
    printf("Error : undefined physical location for section %s\n", s2->name);
    exit(1);
  }

  sv s1addr = s1->physaddr;
  sv s1end = s1addr + s1->physsize;
  sv s2addr = s2->physaddr;
  sv s2end = s2addr + s2->physsize;

  if(s1addr >= s2end || s2addr >= s1end) {
    return 0;
  }

  return 1;
}

int binaries_phys_overlap(section *s1, section *s2)
{
  if(s1->physaddr == -1 || s1->physsize == -1) {
    printf("Error : undefined physical location for section %s\n", s1->name);
    exit(1);
  }

  if(s2->physaddr == -1 || s2->physsize == -1) {
    printf("Error : undefined physical location for section %s\n", s2->name);
    exit(1);
  }

  sv s1addr = s1->physaddr;
  sv s1end = s1addr + s1->physsize;
  sv s2addr = s2->physaddr;
  sv s2end = s2addr + s2->physsize;

  if(s1addr >= s2end || s2addr >= s1end) {
    return 0;
  }

  return 1;
}

symbol *symbol_set(BLSLL(symbol) **symlist, char *symname, chipaddr value, section *section)
{
  symbol *s  = NULL;
  BLSLL(symbol) *sl = NULL;

  if(symlist) {
    sl = *symlist;
    BLSLL_FOREACH(s, sl) {
      if(strcmp(symname, s->name) == 0) {
        break;
      }
    }
  }

  if(!sl) {
    // Symbol not found
    s = symbol_find(symname);

    if(!s) {
      if(symlist) {
        *symlist = blsll_create_symbol(*symlist);
        s = (*symlist)->data;
      } else {
        s = (symbol *)calloc(1, sizeof(symbol));
      }

      s->name = strdup(symname);
      s->value.chip = chip_none;
      s->value.addr = -1;
      s->section = section;

      if(!symlist || *symlist != symbols) {
        symbols = blsll_insert_symbol(symbols, s);
      }
    } else {
      if(symlist) {
        *symlist = blsll_insert_symbol(*symlist, s);
      }
    }
  }

  if(!s) {
    printf("Could not create symbol %s\n", symname);
    exit(1);
  }

  if(!s->section) {
    // The symbol section was undefined
    s->section = section;
  } else if(section && section != s->section) {
    printf("Warning : symbol %s multiply defined (%s and %s at least).\n", symname, section->name, s->section->name);
  }

  if(value.addr != -1) {
    s->value = value;
  }

  return s;
}

symbol *symbol_def(BLSLL(symbol) **symlist, char *symname, section *section)
{
  chipaddr ca = {chip_none, -1};
  return symbol_set(symlist, symname, ca, section);
}

symbol *symbol_set_bus(BLSLL(symbol) **symlist, char *symname, busaddr value, section *section)
{
  chipaddr ca = bus2chip(value);
  return symbol_set(symlist, symname, ca, section);
}

symbol *symbol_set_addr(BLSLL(symbol) **symlist, char *symname, sv value, section *section)
{
  chipaddr ca = bank2chip(&section->source->banks, value);
  return symbol_set(symlist, symname, ca, section);
}

void *merge_lists(void *target, void *source)
{
  BLSLL(section) *rl = (BLSLL(section) *)target;
  BLSLL(section) *sl = (BLSLL(section) *)source;
  section *t, *s;

  BLSLL_FOREACH(s, sl) {
    BLSLL(section) *tl = rl;
    BLSLL_FOREACH(t, tl) {
      if(t == s) {
        break;
      }
    }

    if(!tl) {
      rl = blsll_insert_section(rl, s);
    }
  }

  return rl;
}

void group_dump(const group *grp, FILE *out)
{
  BLSLL(section) *secl;
  section *sec;

  if(grp->file) {
    fprintf(out, " - file `%s`\n", grp->file);
  }

  fprintf(out, " - format %s\n", format_names[grp->format]);

  if(grp->optimize >= 0) {
    fprintf(out, " - optimize %d\n", grp->optimize);
  }

  if(grp->banks.bus < bus_max) {
    fprintf(out, " - bus %s\n\n", bus_names[grp->banks.bus]);
  }

  int c;

  for(c = 0; c < chip_max; ++c) {
    if(grp->banks.bank[c] != -1) {
      fprintf(out, " - bank %s:%u\n", chip_names[c], grp->banks.bank[c]);
    }
  }

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
      busaddr ba = chip2bus(sym->value, bus_main);
      fprintf(out, "0x%08X 0x%06X", (uint32_t)sym->value.addr, 0xFFFFFFU & (uint32_t)ba.addr);
    } else {
      fprintf(out, "?????????? ????????");
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

      if(sec->source && chipaddr_reachable(sec->symbol->value, &sec->source->banks)) {
        busaddr ba = chip2bus(sec->symbol->value, sec->source->banks.bus);

        if(ba.addr != -1) {
          fprintf(out, " ($%08X bus=%s bank=%d)", (unsigned int)ba.addr, bus_names[sec->source->banks.bus], ba.bank);
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

void output_dump(FILE *out)
{
  fprintf(out, " - target %s\n", target_names[maintarget]);

  if(mainout.region) {
    fprintf(out, " - region %s\n", mainout.region);
  }

  if(mainout.file) {
    fprintf(out, " - file `%s`\n", mainout.file);
  }

  if(maintarget == target_scd1 || maintarget == target_scd2) {
    if(mainout.ip) {
      fprintf(out, " - ip `%s`\n", mainout.ip->name);
    }

    if(mainout.sp) {
      fprintf(out, " - sp `%s`\n", mainout.sp->name);
    }
  } else if(mainout.entry) {
    busaddr ba = chip2bus(mainout.entry->value, bus_main);
    fprintf(out, " - entry `%s` (0x%06X)\n", mainout.entry->name, (unsigned int)ba.addr);
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

void blsconf_dump(FILE *out)
{
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
  fprintf(out, "\nOutput\n======\n\n");
  output_dump(out);
  fprintf(out, "\n");

  // Dump symbol values
  fprintf(out, "\n----------------------------------------\n");
  fprintf(out, "\nSymbols\n=======\n\n");
  symtable_dump(symbols, out);
  fprintf(out, "\n");
}

void gen_bol(group *bin)
{
  BLSLL(section) *secl;
  section *sec;
  BLSLL(group) *grpl;
  group *grp;

  // Check if the source is already in the BOL
  grpl = usedbinaries;
  BLSLL_FOREACH(grp, grpl) {
    if(grp == bin) {
      // Binary already processed
      return;
    }
  }

  usedbinaries = blsll_append_group(usedbinaries, bin);

  secl = bin->provides;
  BLSLL_FOREACH(sec, secl) {
    usedsections = blsll_append_section(usedsections, sec);
    mainout.bol = blsll_insert_unique_group(mainout.bol, sec->source);
  }

  grpl = bin->loads;
  BLSLL_FOREACH(grp, grpl) {
    gen_bol(grp);
  }
}

void bls_gen_bol()
{
  if(maintarget == target_scd1 || maintarget == target_scd2) {
    gen_bol(mainout.ipbin);
    gen_bol(mainout.spbin);
  } else {
    gen_bol(mainout.entrybin);
  }
}

void bls_check_binary_load()
{
  // Checks that all used binaries are loaded
  BLSLL(group) *loadedbinl = usedbinaries;
  group *loadedbin;
  BLSLL_FOREACH(loadedbin, loadedbinl) {
    BLSLL(group) *usedbinl = loadedbin->uses_binaries;
    group *usedbin;
    BLSLL_FOREACH(usedbin, usedbinl) {
      // Check that usedbin appears in usedbinaries
      BLSLL(group) *ubl = usedbinaries;
      group *ub;
      BLSLL_FOREACH(ub, ubl) {
        if(ub == usedbin) {
          goto bls_check_binary_load_bin_ok;
        }
      }

      printf("Binary %s is used by %s, but is never loaded.\n", usedbin->name, loadedbin->name);
      exit(1);

bls_check_binary_load_bin_ok:
      continue;
    }
  }
}

void bls_find_entry()
{
  switch(maintarget) {
  default: {
    mainout.entry = symbol_find("MAIN");

    if(!mainout.entry) {
      mainout.entry = symbol_find("main");
    }

    if(!mainout.entry) {
      mainout.entry = symbol_find("MAIN_ASM");
    }

    if(!mainout.entry) {
      printf("Could not find MAIN entry point.\n");
      exit(1);
    }

    mainout.entrybin = 0;
    BLSLL(group) *binl = binaries;
    group *bin;
    BLSLL_FOREACH(bin, binl) {
      BLSLL(section) *secl = bin->provides;
      section *sec;
      BLSLL_FOREACH(sec, secl) {
        BLSLL(symbol) *syml = sec->intsym;
        symbol *sym;
        BLSLL_FOREACH(sym, syml) {
          if(sym == mainout.entry) {
            mainout.entrybin = bin;
            goto bls_find_entry_default_exit;
          }
        }
      }
    }
bls_find_entry_default_exit:

    if(!mainout.entrybin) {
      printf("Could not find binary providing the MAIN entry point.\n");
      exit(1);
    }

    break;
  }

  case target_scd1:
  case target_scd2: {
    symbol *sym;
    BLSLL(section) *secl;
    section *sec;
    BLSLL(group) *binl;
    group *bin;

    // Find IP entry point

    sym = symbol_find("IP_MAIN");

    if(!sym) {
      sym = symbol_find("ip_main");
    }

    if(sym) {
      mainout.ip = sym->section;
    }

    if(!mainout.ip) {
      printf("Could not find IP entry point.\n");
      exit(1);
    }

    binl = binaries;
    BLSLL_FOREACH(bin, binl) {
      BLSLL(section) *secl = bin->provides;
      BLSLL_FOREACH(sec, secl) {
        if(sec == mainout.ip) {
          mainout.ipbin = bin;
          goto bls_find_entry_scd_ipbin_found;
        }
      }
    }
    printf("Could not find IP binary.\n");
    exit(1);
bls_find_entry_scd_ipbin_found:

    // Find SP entry point

    sym = symbol_find("SP_MAIN");

    if(!sym) {
      sym = symbol_find("sp_main");
    }

    if(sym) {
      mainout.sp = sym->section;
    }

    if(!mainout.sp) {
      printf("Could not find SP entry point.\n");
      exit(1);
    }

    binl = binaries;
    BLSLL_FOREACH(bin, binl) {
      BLSLL(section) *secl = bin->provides;
      BLSLL_FOREACH(sec, secl) {
        if(sec == mainout.sp) {
          mainout.spbin = bin;
          goto bls_find_entry_scd_spbin_found;
        }
      }
    }
    printf("Could not find SP binary.\n");
    exit(1);
bls_find_entry_scd_spbin_found:

    break;
  }
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

    case format_asmx:
      source_get_symbols_asmx(grp);

    default:
      break;
    }
  }

  // Generate _SIZE, _PHYS and _PHYSSIZE symbols for sections
  BLSLL(section) *secl = sections;
  section *sec;
  BLSLL_FOREACH(sec, secl) {
    char s[1024];

    chipaddr nullca;
    nullca.chip = chip_none;
    nullca.addr = 0;

    strcpy(s, sec->symbol->name);
    strcat(s, "_SIZE");
    symbol_set(&sec->intsym, s, nullca, sec);

    strcpy(s, sec->symbol->name);
    strcat(s, "_PHYSSIZE");
    symbol_set(&sec->intsym, s, nullca, sec);

    if(maintarget != target_scd1 && maintarget != target_scd2) {
      nullca.chip = maintarget == target_ram ? chip_ram : chip_cart;
    }

    strcpy(s, sec->symbol->name);
    strcat(s, "_PHYS");
    symbol_set(&sec->intsym, s, nullca, sec);
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

    case format_asmx:
      source_get_symbol_values_asmx(grp);
      break;

    default:
      break;
    }
  }

  // Update _SIZE symbols with approx values
  BLSLL(section) *secl;
  section *sec;

  secl = sections;
  BLSLL_FOREACH(sec, secl) {
    char s[1024];

    chipaddr nullca;
    nullca.chip = chip_none;
    nullca.addr = sec->size;

    strcpy(s, sec->symbol->name);
    strcat(s, "_SIZE");
    symbol_set(&sec->intsym, s, nullca, sec);
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

    case format_png:
      source_compile_png(grp);
      break;

    case format_asmx:
      source_compile_asmx(grp);
      break;

    default:
      break;
    }
  }

  // Update _SIZE symbols with final values
  BLSLL(section) *secl = sections;
  section *sec;
  BLSLL_FOREACH(sec, secl) {
    char s[1024];

    chipaddr nullca;
    nullca.chip = chip_none;
    nullca.addr = sec->size;

    strcpy(s, sec->symbol->name);
    strcat(s, "_SIZE");
    symbol_set(&sec->intsym, s, nullca, sec);
  }
}

void bls_cart_to_ram()
{
  BLSLL(symbol) *syml;
  symbol *sym;

  syml = symbols;
  BLSLL_FOREACH(sym, syml) {
    if(sym->value.chip == chip_cart) {
      sym->value.chip = chip_ram;
    }
  }

  BLSLL(section) *secl;
  section *sec;

  secl = sections;
  BLSLL_FOREACH(sec, secl) {
    if(sec->symbol->value.chip == chip_cart) {
      sec->symbol->value.chip = chip_ram;
    }
  }
}

void bls_map_section(section *sec)
{
  if(sec->fixed) {
    // Fixed position section, do not remap
    return;
  }

  if(sec->size < 0) {
    printf("Error : section %s has an unknown size\n", sec->name);
    exit(1);
  } else if(!sec->size) {
    // Sections with null size don't need mapping
    printf("Section %s is empty : no mapping.\n", sec->name);
    return;
  }

  if(sec->symbol->value.chip == chip_none) {
    printf("Error : could not determine chip of section %s\n", sec->name);
    exit(1);
  }

  if(sec->symbol->value.addr != -1) {
    // Section address already known
    printf("Section %s already mapped at %06X.\n", sec->name, (uint32_t)sec->symbol->value.addr);
    return;
  }

  // Find a place for this section
  sv chipstart = chip_start(sec->symbol->value.chip);
  sv chipend = chipstart + chip_size(sec->symbol->value.chip);
  sec->symbol->value.addr = align_value(chipstart, sec->align);
  printf("Section %s(%s) : size=%04X chip=%s chipstart=%06X chipend=%06X ", sec->name, sec->symbol->name, (unsigned int)sec->size, chip_names[sec->symbol->value.chip], (unsigned int)chipstart, (unsigned int)chipend);

  while(sec->symbol->value.addr + sec->size <= chipend) {
    BLSLL(section) *sl = usedsections;
    section *s;
    BLSLL_FOREACH(s, sl) {
      if(s == sec || s->size == 0 || s->symbol->value.addr == -1) {
        continue;
      }

      if(sections_overlap(sec, s)) {
        sec->symbol->value.addr = align_value(s->symbol->value.addr + s->size, sec->align);
        chip_align(&sec->symbol->value);
        goto bls_map_next_section_addr;
      }
    }
    // Found
    printf("mapped at %06X\n", (unsigned int)sec->symbol->value.addr);
    return;
bls_map_next_section_addr:
    continue;
  }

  if(sec->symbol->value.addr + sec->size >= chipend) {
    printf("Error : could not map section %s : not enough memory in chip %s\n", sec->name, chip_names[sec->symbol->value.chip]);
    exit(1);
  }

}

void bls_map()
{
  // Do logical section mapping
  BLSLL(section) *secl;
  section *sec;

  printf("Map logical addresses.\n");

  if(maintarget == target_scd1 || maintarget == target_scd2) {
    // Premap IP at fixed location
    secl = mainout.ipbin->provides;
    sv addr = IPOFFSET;
    printf("Premapping IP binary\n");
    BLSLL_FOREACH(sec, secl) {
      if(sec->format != format_empty && sec->symbol->value.chip != chip_ram) {
        printf("Error: Bootloader binary must be in ram, but section %s is not (it is in %s).\n", sec->name, chip_names[sec->symbol->value.chip]);
        exit(1);
      }
      if(sec->format == format_raw && sec->size > 0) {
        if(sec->symbol->value.addr == -1) {
          printf("Premapping section %s at %06X-%06X\n", sec->name, (u32)addr, (u32)(addr + sec->size - 1));
          sec->symbol->value.addr = addr;
        } else if(sec->symbol->value.addr != addr) {
          printf("Error : invalid offset for section %s (set at %06X, should be %06X).\n", sec->name, (unsigned int)sec->symbol->value.addr, (unsigned int)addr);
          exit(1);
        } else {
          printf("Section %s already premapped at %06X\n", sec->name, (unsigned int)sec->symbol->value.addr);
        }
        addr += sec->size;
      } else if(sec->size > 0 && sec->format != format_empty) {
        printf("Error: format %s is not supported in bootloader binaries\n", format_names[sec->format]);
        exit(1);
      }
    }

    // Premap SP at fixed location
    secl = mainout.spbin->provides;
    addr = SPOFFSET;
    printf("Premapping SP binary\n");
    BLSLL_FOREACH(sec, secl) {
      if(sec->format != format_empty && sec->symbol->value.chip != chip_pram) {
        printf("Error: Bootloader binary must be in pram, but section %s is not (it is in %s).\n", sec->name, chip_names[sec->symbol->value.chip]);
        exit(1);
      }
      if(sec->format == format_raw && sec->size > 0) {
        if(sec->symbol->value.addr == -1) {
          printf("Premapping section %s at %06X-%06X\n", sec->name, (u32)addr, (u32)(addr + sec->size - 1));
          sec->symbol->value.addr = addr;
        } else if(sec->symbol->value.addr != addr) {
          printf("Error : invalid offset for section %s (set at %06X, should be %06X).\n", sec->name, (unsigned int)sec->symbol->value.addr, (unsigned int)addr);
          exit(1);
        } else {
          printf("Section %s already premapped at %06X\n", sec->name, (unsigned int)sec->symbol->value.addr);
        }
        addr += sec->size;
      } else if(sec->size > 0 && sec->format != format_empty) {
        printf("Error: format %s is not supported in bootloader binaries\n", format_names[sec->format]);
        exit(1);
      } else {
        printf("Skipping section %s\n", sec->name);
      }
    }
  }

  secl = usedsections;
  BLSLL_FOREACH(sec, secl) {
    bls_map_section(sec);
  }
}

void bls_map_reset()
{
  // Reset all automatically mapped sections

  BLSLL(section) *secl;
  section *sec;

  secl = usedsections;
  BLSLL_FOREACH(sec, secl) {
    if(!sec->fixed && sec->symbol) {
      sec->symbol->value.addr = -1;
    }
  }
}

void bls_physmap_cart()
{
  chipaddr chipaddrstart = {chip_cart, chip_start(chip_cart)};
  chipaddr chipaddrend = {chip_cart, chip_start(chip_cart) + chip_size(chip_cart)};
  sv chipstart = chip2phys(chipaddrstart);
  sv chipend = chip2phys(chipaddrend);

  // Do logical section mapping
  BLSLL(section) *secl = usedsections;
  section *sec;
  BLSLL_FOREACH(sec, secl) {
    if(sec->physsize == -1) {
      printf("Error : %s has unknown size", sec->name);
      exit(1);
    }

    if(sec->physsize == 0) {
      // Sections with null size don't need mapping
      sec->physaddr = -1;
      continue;
    }

    if(sec->symbol->value.chip == chip_cart || (maintarget == target_ram && sec->symbol->value.chip == chip_ram)) {
      // Section on cartridge are already mapped
      sec->physaddr = chip2phys(sec->symbol->value);

      if(sec->physaddr < chipstart || sec->physaddr + sec->physsize > chipend) {
        printf("Error : Mapped physical address %06X out of range (%06X-%06X)\n", (unsigned int)sec->physaddr, (unsigned int)chipstart, (unsigned int)chipend);
        exit(1);
      }

      continue;
    }

    if(sec->physaddr != -1) {
      // Section address already known
      if(sec->physaddr < chipstart || sec->physaddr + sec->physsize > chipend) {
        printf("Error : Manually specified physical address %06X out of range (%06X-%06X)\n", (unsigned int)sec->physaddr, (unsigned int)chipstart, (unsigned int)chipend);
        exit(1);
      }

      continue;
    }

    // Find a place for this section
    sec->physaddr = chipstart;
    printf("Section %s : size=%04X chipstart=%06X chipend=%06X\n", sec->name, (unsigned int)sec->physsize, (unsigned int)chipstart, (unsigned int)chipend);

    while(sec->physaddr + sec->physsize <= chipend) {
      BLSLL(section) *sl = usedsections;
      section *s;
      BLSLL_FOREACH(s, sl) {
        if(s == sec || s->physsize == 0 || s->physaddr == -1) {
          continue;
        }

        printf("Overlaps ? %s(%06X-%06X) <-> %s(%06X-%06X)  ", sec->name, (uint32_t)sec->symbol->value.addr, (uint32_t)(sec->symbol->value.addr + sec->size - 1), s->name, (uint32_t)s->symbol->value.addr, (uint32_t)(s->symbol->value.addr + s->size - 1));

        if(phys_overlap((element *)sec, (element *)s)) {
          printf("YES : %06X => ", (unsigned int)sec->physaddr);
          sec->physaddr = align_value(s->physaddr + s->physsize, 2);

          if(sec->physaddr < chipstart || sec->physaddr + sec->physsize > chipend) {
            printf("Error : Physical address %06X out of range (%06X-%06X)\n", (unsigned int)sec->physaddr, (unsigned int)chipstart, (unsigned int)chipend);
            exit(1);
          }

          printf("%06X\n", (unsigned int)sec->physaddr);
          goto bls_map_next_section_addr;
        }
      }
      // Found
      printf("NO\n");
      goto bls_map_next_section;
bls_map_next_section_addr:
      continue;
    }

    if(sec->physaddr + sec->physsize >= chipend) {
      printf("Error : could not map section %s : not enough memory in cart\n", sec->name);
      exit(1);
    }

bls_map_next_section:
    continue;
  }

  secl = usedsections;
  BLSLL_FOREACH(sec, secl) {
    char s[1024];

    chipaddr nullca = phys2chip(sec->physaddr);

    strcpy(s, sec->symbol->name);
    strcat(s, "_PHYS");
    symbol_set(&sec->intsym, s, nullca, sec);
  }
}

void bls_physmap_cd()
{
  if(!mainout.ip || !mainout.sp || !mainout.ipbin || !mainout.spbin || mainout.ipbin->physsize <= 0 || mainout.spbin->physsize <= 0) {
    printf("Error : could not find valid IP and SP binaries.\n");
    exit(1);
  }

  if(mainout.ip->symbol->value.chip != chip_ram || mainout.ip->symbol->value.addr != IPOFFSET) {
    printf("Error : Invalid IP location.\n");
    exit(1);
  }

  if(mainout.sp->symbol->value.chip != chip_pram || mainout.sp->symbol->value.addr != SPOFFSET) {
    printf("Error : Invalid SP location.\n");
    exit(1);
  }

  // Map IP on physical medium
  mainout.ipbin->physaddr = CDHEADERSIZE + SECCODESIZE + 6;

  if(mainout.ipbin->physaddr + mainout.ipbin->physsize > 16 * CDBLOCKSIZE) {
    printf("Error: IP too large.\n");
    exit(1);
  }

  // Map SP on physical medium
  mainout.spbin->physaddr = align_value(mainout.ipbin->physaddr + mainout.ipbin->physsize, CDBLOCKSIZE) + SPHEADERSIZE;
  if(mainout.contentsize && mainout.spbin->physaddr - SPHEADERSIZE < 16 * CDBLOCKSIZE && mainout.spbin->physaddr + mainout.spbin->physsize > 16 * CDBLOCKSIZE) {
    // SP overlaps content: relocate after ISO9660 content
    mainout.spbin->physaddr = align_value(mainout.contentsize, CDBLOCKSIZE) + SPHEADERSIZE;
  }

  // Map other binaries on physical medium
  sv physstart;
  if(mainout.contentsize && mainout.spbin->physaddr < 16 * CDBLOCKSIZE) {
    // SP is before ISO filesystem, put content after the filesystem
    physstart = align_value(mainout.contentsize, CDBLOCKSIZE);
  } else {
    // No filesystem or SP after the filesystem: put content after SP
    physstart = align_value(mainout.spbin->physaddr + mainout.spbin->physsize, CDBLOCKSIZE);
  }
  sv physend = MAXCDBLOCKS * CDBLOCKSIZE;

  BLSLL(group) *binl = usedbinaries;
  group *bin;
  BLSLL_FOREACH(bin, binl) {
    if(bin == mainout.ipbin || bin == mainout.spbin) {
      continue;
    }

    if(bin->physsize == -1) {
      printf("Error : %s has unknown size.\n", bin->name);
      exit(1);
    }

    if(bin->physsize == 0) {
      bin->physaddr = -1;
      continue;
    }

    // Find a place for this binary
    bin->physaddr = physstart;
    printf("Binary %s : size=%04X physstart=%06X physend=%06X\n", bin->name, (unsigned int)bin->physsize, (unsigned int)physstart, (unsigned int)physend);

    if(bin->physaddr + bin->physsize >= physend) {
      printf("Error : could not map binary %s : CD-ROM full\n", bin->name);
      exit(1);
    }

    physstart = align_value(bin->physaddr + bin->physsize, CDBLOCKSIZE);
  }

  // Store image size
  mainout.imagesize = physstart;
  if(mainout.imagesize < MINCDBLOCKS * CDBLOCKSIZE) {
    mainout.imagesize = MINCDBLOCKS * CDBLOCKSIZE;
  }

  binl = usedbinaries;
  BLSLL_FOREACH(bin, binl) {
    char *s = symname2(bin->name, "_SECTOR");
    chipaddr secca = {chip_none, bin->physaddr / CDBLOCKSIZE};
    symbol_set(NULL, s, secca, NULL);
    free(s);

    s = symname2(bin->name, "_SECCNT");
    chipaddr szca = {chip_none, (bin->physsize + CDBLOCKSIZE - 1) / CDBLOCKSIZE};
    symbol_set(NULL, s, szca, NULL);
    free(s);
  }
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
          grp->provides = blsll_insert_unique_section(grp->provides, s);
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

  if(maintarget == target_ram) {
    // Remap all cart symbols to ram
    BLSLL(symbol) *syml = symbols;
    symbol *sym;
    BLSLL_FOREACH(sym, syml) {
      if(sym->value.chip == chip_cart) {
        sym->value.chip = chip_ram;
      }
    }
  }

  if(maintarget != target_scd1 && maintarget != target_scd2) {
    if(!mainout.mainstack) {
      sections = blsll_create_section(sections);
      mainout.mainstack = sections->data;
      mainout.mainstack->name = strdup("mainstack");
      mainout.mainstack->format = format_empty;
    }

    if(!mainout.mainstack->symbol) {
      symbols = blsll_create_symbol(symbols);
      mainout.mainstack->symbol = symbols->data;
      mainout.mainstack->symbol->name = strdup("MAINSTACK");
      mainout.mainstack->symbol->value.chip = chip_ram;
      mainout.mainstack->symbol->value.addr = -1;
    }

    if(mainout.mainstack->symbol->value.addr == -1) {
      // Map stack to a default place
      mainout.mainstack->symbol->value.chip = chip_ram;
      mainout.mainstack->symbol->value.addr = MAINSTACK - MAINSTACKSIZE;
      mainout.mainstack->size = MAINSTACKSIZE;
      mainout.mainstack->format = format_empty;
    }
  } else {
    // Map ip sections to their final location
  }

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

    case format_asmx:
      printf("Premapping %s\n", grp->name);
      source_premap_asmx(grp);
      break;

    default:
      break;
    }
  }
}

void bls_pack_binaries()
{
  // Packs each section into separate files and compress them
  BLSLL(group) *binl = usedbinaries;
  group *bin;
  BLSLL_FOREACH(bin, binl) {
    char binname[4096];
    snprintf(binname, 4096, BUILDDIR"/%s.binary", bin->name);
    sections_cat(bin, binname);

    if(!bin->physsize) {
      continue;
    }

    printf("Packing binary %s\n", bin->name);
    snprintf(binname, 4096, "%s.binary", bin->name);

    switch(bin->format) {
    default:
    case format_raw: {
      size_t fs = pack_raw(binname);
      printf("Size of %s : %04X\n", bin->name, (unsigned int)fs);
      bin->physsize = fs;
      break;
    }

    case format_empty:
    case format_zero: {
      bin->physsize = 0;
      break;
    }
    }
  }

  // Update _PHYSSIZE and _OFFSET symbols with final values
  BLSLL(section) *secl = sections;
  section *sec;
  BLSLL_FOREACH(sec, secl) {
    char s[1024];

    chipaddr nullca;
    nullca.chip = chip_none;
    nullca.addr = sec->physsize;

    strcpy(s, sec->symbol->name);
    strcat(s, "_PHYSSIZE");
    symbol_set(&sec->intsym, s, nullca, sec);

    nullca.addr = sec->physaddr;
    strcpy(s, sec->symbol->name);
    strcat(s, "_BINOFFSET");
    symbol_set(&sec->intsym, s, nullca, sec);
  }
}

void bls_pack_sections()
{
  // Packs each section into separate files and compress them
  BLSLL(section) *secl = sections;
  section *sec;
  BLSLL_FOREACH(sec, secl) {
    if(!sec->size || !sec->physsize) {
      // Ignore empty sections
      sec->physsize = 0;
      continue;
    }

    printf("Packing section %s\n", sec->name);

    switch(sec->format) {
    default:
    case format_raw: {
      size_t fs = pack_raw(sec->name);
      printf("Size of %s : %04X\n", sec->name, (unsigned int)fs);
      sec->physsize = fs;
      break;
    }

    case format_empty:
    case format_zero: {
      sec->physsize = 0;
      break;
    }
    }
  }

  // Update _PHYSSIZE symbols with final values
  BLSLL_FOREACH(sec, secl) {
    char s[1024];

    chipaddr nullca;
    nullca.chip = chip_none;
    nullca.addr = sec->physsize;

    strcpy(s, sec->symbol->name);
    strcat(s, "_PHYSSIZE");
    symbol_set(&sec->intsym, s, nullca, sec);
  }
}


void bls_build_genesis_header(FILE *f)
{
  if(maintarget == target_gen) {
    if(mainout.region && mainout.region[0] == 'U') {
      fprintf(f, "SEGA GENESIS    ");
    } else {
      fprintf(f, "SEGA MEGA DRIVE ");
    }
  } else if(maintarget == target_ram) {
    fprintf(f, "SEGA RAM PROGRAM");
  } else if(maintarget == target_scd1 || maintarget == target_scd2) {
    fprintf(f, "SEGA CD         ");
  }

  fprintf(f, "%-16s", mainout.copyright);
  fprintf(f, "%-16s", mainout.name);
  fprintf(f, "%-16s", mainout.name);
  fprintf(f, "%-16s", mainout.serial);
  fprintf(f, "%-96s", "J"); // Joystick type
  fprintf(f, "%-16s", mainout.region); // region
}


void bls_compute_content_size()
{
  if(mainout.iso9660 && *mainout.iso9660) {
    FILE *nul = fopen("/dev/null", "w");
    mainout.contentsize = blsiso_gen(nul, mainout.name, mainout.serial, mainout.copyright, strcasecmp(mainout.iso9660, "self.iso") ? mainout.iso9660 : NULL, 0, 0);
  }
}


void bls_build_cd_image()
{
  printf("Writing CD image to %s\n", mainout.file);

  FILE *f = fopen(mainout.file, "w");

  if(!mainout.region) {
    mainout.region = strdup("U");
  }

  fprintf(f, "SEGADISCSYSTEM  ");
  fprintf(f, "           "); // TITLE
  fwrite("\x00\x00\x00\x00\x01", 1, 5, f);
  fprintf(f, "           "); // AUTHOR
  fwrite("\x00\x00\x00\x00\x00", 1, 5, f);

  if(CDHEADERSIZE + mainout.ipbin->physsize + SECCODESIZE + 6 <= CDBLOCKSIZE) {
    // Small IP
    fputlong(0x200, f); // offset
    fputlong(0x600, f); // size
  } else {
    // Big IP : more than one sector
    // Offset does not match since the Sega CD BIOS is completely buggy
    fputlong(0x800, f); // offset
    fputlong(align_value(mainout.ipbin->physsize + SECCODESIZE + 6, CDBLOCKSIZE) - 0x800, f); // size
  }

  fputlong(0, f);
  fputlong(0, f);

  fputlong(mainout.spbin->physaddr - SPHEADERSIZE, f);
  fputlong(align_value(mainout.spbin->physsize + SPHEADERSIZE, CDBLOCKSIZE), f);
  fputlong(0, f);
  fputlong(0, f);

  int i;

  for(i = 0; i < 176; ++i) {
    fputc(' ', f);
  }

  bls_build_genesis_header(f);

  if(ftell(f) != CDHEADERSIZE) {
    printf("Warning: genesis header does not have the correct size.\n");
  }

  // Output security code before IP
  switch(mainout.region[0]) {
  case 'U':
    printf("Using security code for US region (length = %X).\n", (unsigned int)sizeof(scdboot_us));
    fwrite(scdboot_us, 1, sizeof(scdboot_us), f);
    break;

  case 'E':
    printf("Using security code for EU region (length = %X).\n", (unsigned int)sizeof(scdboot_eu));
    fwrite(scdboot_eu, 1, sizeof(scdboot_eu), f);
    break;

  case 'J':
    printf("Using security code for JP region (length = %X).\n", (unsigned int)sizeof(scdboot_jp));
    fwrite(scdboot_jp, 1, sizeof(scdboot_jp), f);
    break;
  }

  sv ipmain = symbol_get_bus("IP_MAIN", bus_main);

  if(ipmain == -1) {
    ipmain = symbol_get_bus("ip_main", bus_main);
  }

  if(ftell(f) & 1) {
    printf("Error : odd number of bytes in security code.\n");
    exit(1);
  }

  if(ftell(f) < CDHEADERSIZE + SECCODESIZE - 6) {
    fwrite("\x4E\xF9", 1, 2, f); // Skip padding and JMP directly to the IP entry point after security code
    fputlong(ipmain, f);
  }

  // Fill in-between code with NOP
  for(i = ftell(f); i < CDHEADERSIZE + SECCODESIZE; i += 2) {
    fwrite("\x4E\x71", 1, 2, f);
  }

  fwrite("\x4E\xF9", 1, 2, f); // JMP to the IP entry point
  fputlong(ipmain, f);

  // Output SP header
  fseek(f, mainout.spbin->physaddr - SPHEADERSIZE, SEEK_SET);

  if(ftell(f) & (CDBLOCKSIZE - 1)) {
    printf("Warning : SP header is not aligned on a sector boundary : current offset = %08X.\n", (unsigned int)ftell(f));
  }

  // 0x00 program name
  fprintf(f, "MAIN       ");
  fputc(0, f);

  // 0x0C Version
  fputword(0, f);

  // 0x0E Type
  fputword(0, f);

  // 0x10 Next header
  fputlong(0, f);

  // 0x14 Length
  fputlong(mainout.spbin->physsize + SPHEADERSIZE, f);

  // 0x18 Entry point table offset
  fputlong(0x20, f);
  fputlong(0x00, f);

  sv spmain = symbol_get_bus("SP_MAIN", bus_sub);

  if(spmain == -1) {
    spmain = symbol_get_bus("sp_main", bus_sub);
  }

  if(spmain == -1) {
    printf("Error : cannot find sp_main symbol\n");
    exit(1);
  }

  sv spinit = symbol_get_bus("SP_INIT", bus_sub);

  if(spinit == -1) {
    spinit = symbol_get_bus("sp_init", bus_sub);
  }

  if(spinit == -1) {
    spinit = spmain;
  }

  sv spl2 = symbol_get_bus("SUB_HWINT_LEVEL2", bus_sub);

  if(spl2 == -1) {
    spl2 = symbol_get_bus("SUB_INT_LEVEL2", bus_sub);
  }

  if(spinit > 0x6020 + 0xFFFF) {
    printf("SP_INIT is too far from the beginning of the SP binary file.\n");
    exit(1);
  }

  if(spmain > 0x6020 + 0xFFFF) {
    printf("SP_MAIN is too far from the beginning of the SP binary file.\n");
    exit(1);
  }

  if(spl2 > 0x6020 + 0xFFFF) {
    printf("SUB LEVEL2 interrupt is too far from the beginning of the SP binary file.\n");
    exit(1);
  }

  // 0x20 Entry points
  fputword(spinit - 0x6020, f);
  fputword(spmain - 0x6020, f);
  fputword(spl2 == -1 ? 0 : spl2 - 0x6020, f);
  fputword(0, f);

  // 0x28 : End of SP header (SPHEADERSIZE)


  // Output binaries
  BLSLL(group) *binl = usedbinaries;
  group *bin;
  BLSLL_FOREACH(bin, binl) {
    if(bin->physaddr == -1) {
      continue;
    }

    char inname[4096];
    snprintf(inname, 4096, BUILDDIR"/%s.binary.phy", bin->name);

    FILE *i = fopen(inname, "r");

    if(!i) {
      printf("Error : cannot find physical image %s for binary %s\n", inname, bin->name);
      exit(1);
    }

    fseek(f, bin->physaddr, SEEK_SET);
    filecat(i, f);

    fclose(i);
  }

  // Pad to reach minimum image size
  fseek(f, 0, SEEK_END);

  for(i = ftell(f); i < MINCDBLOCKS * CDBLOCKSIZE; ++i) {
    fputc(0, f);
  }

  // Output iso9660 filesystem
  if(mainout.iso9660 && *mainout.iso9660) {
    blsiso_gen(f, mainout.name, mainout.serial, mainout.copyright, strcasecmp(mainout.iso9660, "self.iso") ? mainout.iso9660 : NULL, mainout.imagesize, 1);
  }
}


void bls_build_cart_image()
{
  printf("Writing image to %s\n", mainout.file);
  FILE *f = fopen(mainout.file, "w");

  // Write data

  BLSLL(section) *secl = usedsections;
  section *sec;
  BLSLL_FOREACH(sec, secl) {
    if(sec->physaddr == -1 || sec->physsize == 0) {
      continue;
    }

    char inname[4096];
    snprintf(inname, 4096, BUILDDIR"/%s.phy", sec->name);
    FILE *i = fopen(inname, "r");

    if(!i) {
      printf("Error : cannot find physical image %s for section %s\n", inname, sec->name);
      exit(1);
    }

    fseek(f, sec->physaddr, SEEK_SET);
    filecat(i, f);

    fclose(i);
  }

  if(maintarget == target_ram) {
    fseek(f, 0, SEEK_END);
    sv i;

    for(i = ftell(f); i < 0xFF00; ++i) {
      fputc('\xFF', f);
    }
  }

  const char *ramloader =
    // Unlock TMSS
    "\x10\x39\x00\xA1\x00\x01"
    "\x02\x00\x00\x0F"
    "\x67\x08"
    "\x23\xF8\x01\x00\x00\xA1\x40\x00"

    // Copy program to RAM
    "\x41\xF8\x02\x00"
    "\x43\xF9\x00\xFF\x00\x00"
    "\x30\x3C\x3F\x3F"
    "\x22\xD8"
    "\x51\xC8\xFF\xFC"

    // Jump to the entry point
    "\x4E\xF9";
  sv ramloader_size = 0x2A; // This does not count the 4 bytes of the entry point immediatly following
  sv ramloader_padding = 0x100 - ramloader_size - 4 - 0xC0;

  // Write default stackpointer
  fseek(f, 0x000000, SEEK_SET);
  chipaddr stk = mainout.mainstack->symbol->value;
  stk.addr += mainout.mainstack->size;
  fputaddr(stk, bus_main, f);

  // Write entry point
  if(maintarget == target_ram) {
    fputlong(0x00000C0 + ramloader_padding, f); // Boot on RAM loader
  } else {
    fputaddr(mainout.entry->value, bus_main, f);
  }

  // Write interrupt vectors
  sv addr;
#define INTVECT(name) if((addr = symbol_get_bus("HWINT_"#name, bus_main)) != -1) fputlong(addr, f); else if((addr = symbol_get_bus("INT_"#name, bus_main)) != -1) fputlong(addr, f); else fputaddr(mainout.entry->value, bus_main, f);
#define INTVECT2(name,name2) if((addr = symbol_get_bus("HWINT_"#name, bus_main)) != -1) fputlong(addr, f); else if((addr = symbol_get_bus("INT_"#name, bus_main)) != -1) fputlong(addr, f); else \
                             if((addr = symbol_get_bus("HWINT_"#name2, bus_main)) != -1) fputlong(addr, f); else if((addr = symbol_get_bus("INT_"#name2, bus_main)) != -1) fputlong(addr, f); else fputaddr(mainout.entry->value, bus_main, f);

  INTVECT(BUSERR) // 0x08
  INTVECT(ADDRERR) // 0x0C
  INTVECT(ILL) // 0x10
  INTVECT(ZDIV) // 0x14
  INTVECT(CHK) // 0x18
  INTVECT(TRAPV) // 0x1C
  INTVECT(PRIV) // 0x20
  INTVECT(TRACE) // 0x24
  INTVECT(LINEA) // 0x28
  INTVECT(LINEF) // 0x2C
  unsigned int i;

  for(i = 0; i < 12; ++i) {
    fputaddr(mainout.entry->value, bus_main, f);  // 0x30 .. 0x060
  }

  INTVECT(SPURIOUS) // 0x60
  INTVECT(LEVEL1) // 0x64
  INTVECT2(LEVEL2,PAD) // 0x68
  INTVECT(LEVEL3) // 0x6C
  INTVECT2(LEVEL4,HBLANK) // 0x70
  INTVECT(LEVEL5) // 0x74
  INTVECT2(LEVEL6,VBLANK) // 0x78
  INTVECT(LEVEL7) // 0x7C
  INTVECT(TRAP00) // 0x80
  INTVECT(TRAP01)
  INTVECT(TRAP02)
  INTVECT(TRAP03)
  INTVECT(TRAP04)
  INTVECT(TRAP05)
  INTVECT(TRAP06)
  INTVECT(TRAP07) // 0x9C
  INTVECT(TRAP08) // 0xA0
  INTVECT(TRAP09)
  INTVECT(TRAP10)
  INTVECT(TRAP11)
  INTVECT(TRAP12)
  INTVECT(TRAP13)
  INTVECT(TRAP14)
  INTVECT(TRAP15) // 0xBC

  if(maintarget == target_ram) {
    // Embed RAM loader in reserved interrupt vectors

    // Pad with NOP
    for(i = 0; i < ramloader_padding; i += 2) {
      fputc('\x4E', f);
      fputc('\x71', f);
    }

    // Write loader
    fwrite(ramloader, 1, ramloader_size, f);

    // Finish with entry point
    fputaddr(mainout.entry->value, bus_main, f);
  } else {
    // Reserved interrupt vectors
    for(i = 0; i < 16; ++i) {
      fputaddr(mainout.entry->value, bus_main, f);
    }
  }

#undef INTVECT
#undef INTVECT2

  // Write SEGA header
  fseek(f, 0x000100, SEEK_SET);

  bls_build_genesis_header(f);

  // Compute checksum
  // TODO

  fclose(f);
}


char *path_prefixes[4096] = {
  ".", // Here comes the blsgen.md directory
  BLSPREFIX "/share/blastsdk/src",
  BLSPREFIX "/share/blastsdk/include",
  NULL
};

char include_prefixes[4096];

void gen_include_prefixes()
{
  unsigned int i;
  *include_prefixes = 0;

  for(i = 0; path_prefixes[i]; ++i) {
    strcat(include_prefixes, " -I ");
    strcat(include_prefixes, path_prefixes[i]);
  }
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

int findfile(char *name, const char *f)
{
  strcpy(name, f);

  FILE *fp = fopen(name, "r");

  if(fp) {
    fclose(fp);
    return 1;
  }

  unsigned int i;

  for(i = 0; i < sizeof(path_prefixes) / sizeof(*path_prefixes) && path_prefixes[i]; ++i) {
    sprintf(name, "%s/%s", path_prefixes[i], f);
    FILE *fp = fopen(name, "r");

    if(fp) {
      fclose(fp);
      return 2;
    }
  }

  printf("Warning : %s not found\n", f);
  return 0;
}

void tmpdir(char *out, const char *f)
{
  sprintf(out, BUILDDIR "/%s", f);
}

void output_dump_cart(FILE *f)
{
  // Dump physical mapping
  fprintf(f, "\n----------------------------------------\n\nOutput map\n==========\n\n    offset   size     section\n");

  BLSLL(section) *secl = usedsections;
  section *sec;
  BLSLL_FOREACH(sec, secl) {
    if(sec->physaddr == -1 || sec->physsize == 0) {
      continue;
    }

    char inname[4096];
    snprintf(inname, 4096, BUILDDIR"/%s.phy", sec->name);
    FILE *i = fopen(inname, "r");

    if(!i) {
      continue;
    }

    fseek(i, 0, SEEK_END);
    fprintf(f, "    %08X %08X%s %s\n", (unsigned int)sec->physaddr, (unsigned int)sec->physsize, sec->physsize != ftell(i) ? " (wrong)" : "", sec->name);

    fclose(i);
  }
}

void output_dump_cd(FILE *f)
{
  // Dump physical mapping
  fprintf(f, "\n----------------------------------------\n\nOutput map\n==========\n\n    offset   size     binary\n");

  BLSLL(group) *binl = usedbinaries;
  group *bin;
  BLSLL_FOREACH(bin, binl) {
    if(bin->physaddr == -1 || bin->physsize == 0) {
      continue;
    }

    char inname[4096];
    snprintf(inname, 4096, BUILDDIR"/%s.binary.phy", bin->name);
    FILE *i = fopen(inname, "r");

    if(!i) {
      continue;
    }

    fseek(i, 0, SEEK_END);
    fprintf(f, "\n    %08X %08X%s %s\n", (unsigned int)bin->physaddr, (unsigned int)bin->physsize, bin->physsize != ftell(i) ? " (wrong)" : "", bin->name);

    BLSLL(section) *secl = bin->provides;
    section *sec;
    u32 offset = 0;
    if(bin == mainout.ipbin || bin == mainout.spbin) {
      BLSLL_FOREACH(sec, secl) {
        if(sec->size == 0 || sec->format != format_raw) {
          continue;
        }
        fprintf(f, "     -> %08X %08X %s\n", (unsigned int)sec->physaddr, (unsigned int)sec->physsize, sec->name);
      }
    } else {
      BLSLL_FOREACH(sec, secl) {
        if(sec->physsize <= 0 || sec->format == format_zero || sec->format == format_empty) {
          continue;
        }

        fprintf(f, "     -> %08X %08X %s\n", offset, (unsigned int)sec->physsize, sec->name);
        offset += sec->physsize;

        if(sec->physsize & 1) {
          ++offset;
        }
      }
    }

    fclose(i);
  }
}

void confdump()
{
  FILE *f = fopen(BUILDDIR"/blsgen.md", "w");
  blsconf_dump(f); // Dump full configuration for reference and debugging

  if(maintarget == target_scd1 || maintarget == target_scd2) {
    output_dump_cd(f);
  } else {
    output_dump_cart(f);
  }

  fclose(f);
}

int main(int argc, char **argv)
{
  atexit(confdump);

  mkdir(BUILDDIR, 0777);

  if(argc > 1) {
    char arg[1024];
    strcpy(arg, argv[1]);
    path_prefixes[0] = strdup(dirname(arg));
    gen_include_prefixes();
    blsconf_load(argv[1]);
  } else {
    gen_include_prefixes();
    blsconf_load("blsgen.md");
  }


  bls_get_symbols();
  bls_finalize_binary_dependencies();
  bls_find_entry();
  bls_gen_bol();
  bls_check_binary_load();

  if(maintarget != target_gen) {
    bls_cart_to_ram();
  }

  bls_get_symbols(); // Get binary size with correct binary load
  bls_map();
  bls_get_symbol_values();
  bls_compile(); // Compile with most values to get a good approximation of file sizes

  if(maintarget != target_scd1 && maintarget != target_scd2) {
    bls_pack_sections(); // Pack once to find physical size for all files
    bls_physmap_cart(); // Map physical cartridge image
    bls_map_reset();
    bls_map(); // Remap with final binary sizes
    bls_get_symbol_values();
    bls_compile(); // Recompile with physical addresses
    bls_pack_sections(); // Final packing pass
    bls_build_cart_image(); // Build the final cart image
  } else {
    bls_compute_content_size(); // Compute the size of the iso9660 filesystem
    bls_pack_binaries(); // Pack once to find physical size for all files
    bls_physmap_cd(); // Map physical CD
    bls_map_reset();
    bls_map(); // Remap with final binary sizes
    bls_get_symbol_values();
    bls_compile(); // Compile with physical addresses
    bls_pack_binaries(); // Final packing pass
    bls_build_cd_image(); // Build the final CD image
  }
}


// vim: ts=2 sw=2 sts=2 et
