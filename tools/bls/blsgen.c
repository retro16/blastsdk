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
  group *g = (group *)calloc(1, sizeof(group));
  g->optimize = -1;
  return g;
}

void group_free(group *p) {
  if(p->name) free(p->name);
  free(p);
}

const char bus_names[][8] = {"none", "main", "sub", "z80"};
const char chip_names[][8] = {"none", "stack", "cart", "bram", "zram", "vram", "ram", "pram", "wram", "pcm"};
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

void group_dump(const group *grp, FILE *out) {
  BLSLL(section) *secl;
  section *sec;

  fprintf(out, " - format %s\n", format_names[grp->format]);

  if(grp->optimize >= 0) {
    fprintf(out, " - optimize %d\n", grp->optimize);
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

  if(grp->uses) {
    BLSLL(section) *secl = grp->uses;
    section *sec;

    fprintf(out, " - uses ");
    BLSLL_FOREACH(sec, secl) {
      fprintf(out, "`%s`", sec->name);
      if(secl->next) {
        fprintf(out, ", ");
      }
    }
    fprintf(out, "\n");
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
    fprintf(out, "%5.5s", chip_names[sym->value.chip]);
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
      fprintf(out, " - physalign %d\n", sec->physalign);
    }

    if(sec->symbol) {
      fprintf(out, " - chip %s\n", chip_names[sec->symbol->value.chip]);
      if(sec->symbol->value.addr != -1) {
        fprintf(out, " - addr $%08lX\n", (uint64_t)sec->symbol->value.addr);
      }
    }

    if(sec->align) {
      fprintf(out, " - align %d\n", sec->align);
    }
    if(sec->size >= 0) {
      fprintf(out, " - size $%lX\n", (uint64_t)sec->size);
    }
    fprintf(out, "\n");

    if(sec->symbol) {
      if(sec->symbol->name) {
        fprintf(out, "Symbol name `%s`\n\n", sec->symbol->name);
      }
    }

    if(sec->intsym) {
      fprintf(out, "Internal symbol table :\n");
      symtable_dump(sec->intsym, out);
    }

    if(sec->extsym) {
      fprintf(out, "External symbol table :\n");
      symtable_dump(sec->extsym, out);
    }
}

void output_dump(output *outp, FILE *out) {
  fprintf(out, " - target %s\n", target_names[outp->target]);
  if(outp->region) {
    fprintf(out, " - region %s\n", outp->region);
  }
  if(outp->file) {
    fprintf(out, " - file `%s`\n", outp->file);
  }
  if(outp->target == target_scd) {
    if(outp->ip) {
      fprintf(out, " - ip `%s`\n", outp->ip->name);
    }
    if(outp->sp) {
      fprintf(out, " - sp `%s`\n", outp->sp->name);
    }
  } else if(outp->entry) {
    fprintf(out, " - entry `%s`\n", outp->entry->name);
  }

  const BLSLL(group) *grpl;
  group *grp;
  if(outp->binaries) {
    fprintf(out, " - binaries ");
    grpl = outp->binaries;
    BLSLL_FOREACH(grp, grpl) {
      fprintf(out, "`%s`", grp->name);
      if(grpl->next) {
        fprintf(out, ", ");
      }
    }
    fprintf(out, "\n");
  }

  if(outp->bol) {
    fprintf(out, "\nBuild order list: ");
    grpl = outp->bol;
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
  BLSLL(output) *outl;
  output *outp;

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

  // Dump outputs
  fprintf(out, "\n----------------------------------------\n");
  fprintf(out, "\nOutputs\n=======\n\n");
  outl = outputs;
  BLSLL_FOREACH(outp, outl) {
    fprintf(out, "Output `%s`\n------\n\n", outp->name);
    output_dump(outp, out);
    fprintf(out, "\n");
  }
}

int main() {
  blsconf_load("blsgen.md");

  blsconf_dump(stdout);
}
