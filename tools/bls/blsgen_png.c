#include "blsgen.h"
#include "blsconf.h"

/* blsgen functions to generate binaries from png sources */

void section_create_png(group *source, const mdconfnode *mdconf)
{
  section *s;

  // Generate the .image section
  source->provides = blsll_insert_section(source->provides, (s = section_parse_ext(mdconf, source->name, ".image")));
  s->source = source;

  // Generate the .map section
  source->provides = blsll_insert_section(source->provides, (s = section_parse_ext(mdconf, source->name, ".map")));
  s->source = source;

  // Generate the .palette section
  source->provides = blsll_insert_section(source->provides, (s = section_parse_ext(mdconf, source->name, ".palette")));
  s->source = source;
}

