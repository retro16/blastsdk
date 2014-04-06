#include "blsgen.h"
#include "blsconf.h"

/* blsgen functions to generate binaries from gcc sources */

void section_create_gcc(group *source, const mdconfnode *mdconf)
{
  mdconf = mdconf->child;

  const mdconfnode *mdtext = mdconfsearch(mdconf, "section=.text");
  const mdconfnode *mddata = mdconfsearch(mdconf, "section=.data");
  const mdconfnode *mdbss = mdconfsearch(mdconf, "section=.bss");


  section *s;
  symbol *sym;

  // Generate the .text section
  source->provides = blsll_create_section(source->provides);
  sections = blsll_insert_section(sections, source->provides->data);
  s = source->provides->data;
  section_parse(s, mdtext);
  s->name = strdup(".text");
  s->datafile = symname2(source->name, "text");
  s->source = source;
  symbols = blsll_create_symbol(symbols);
  sym = symbols->data;
  s->symbol = sym;
  sym->name = strdup(s->datafile);
  sym->section = s;


  // Generate the .data section
  source->provides = blsll_create_section(source->provides);
  sections = blsll_insert_section(sections, source->provides->data);
  s = source->provides->data;
  section_parse(s, mddata);
  s->name = strdup(".data");
  s->datafile = symname2(source->name, "data");
  s->source = source;
  symbols = blsll_create_symbol(symbols);
  sym = symbols->data;
  s->symbol = sym;
  sym->name = strdup(s->datafile);
  sym->section = s;


  // Generate the .bss section
  source->provides = blsll_create_section(source->provides);
  sections = blsll_insert_section(sections, source->provides->data);
  s = source->provides->data;
  section_parse(s, mdbss);
  s->name = strdup(".bss");
  s->datafile = symname2(source->name, "bss");
  s->physsize = 0; // Do not store BSS on physical medium
  s->source = source;
  symbols = blsll_create_symbol(symbols);
  sym = symbols->data;
  s->symbol = sym;
  sym->name = strdup(s->datafile);
  sym->section = s;
}

