/* blsgen functions to generate binaries from gcc sources */

void section_create_gcc(source_t *source, const mdconfnode_t *mdconf)
{
  mdconfnode_t *mdtext = mdconfsearch(mdconf, "section=.text");
  mdconfnode_t *mddata = mdconfsearch(mdconf, "section=.data");
  mdconfnode_t *mdbss = mdconfsearch(mdconf, "section=.bss");


  section_t *s;

  // Generate the .text section
  source->sections = blsll_create_section_t(source->sections);
  sections = blsll_insert_section_t(sections, source->sections->data);
  s = source->sections->data;
  parse_section(s, mdtext);
  s->name = strdup(".text");
  s->datafile = symname2(source->name, "text");
  s->symbol.name = strdup(s->datafile);


  // Generate the .data section
  source->sections = blsll_create_section_t(source->sections);
  sections = blsll_insert_section_t(sections, source->sections->data);
  s = source->sections->data;
  parse_section(s, mddata);
  s->name = strdup(".data");
  s->datafile = symname2(source->name, "data");
  s->symbol.name = strdup(s->datafile);


  // Generate the .bss section
  source->sections = blsll_create_section_t(source->sections);
  sections = blsll_insert_section_t(sections, source->sections->data);
  s = source->sections->data;
  parse_section(s, mdtext);
  s->name = strdup(".bss");
  s->datafile = symname2(source->name, "bss");
  s->symbol.name = strdup(s->datafile);
  s->physsize = 0; // Do not store BSS on physical medium
}

