#include "blsgen.h"
#include "mdconf.h"

mdconfnode_t *mdconf = 0;
source_t *sources = 0;
section_t *sections = 0;
output_t *outputs = 0;
symbol_t *symbols = 0;

void blsconf_load(const char *file) {
  mdconf = mdconfparsefile(file);

  mdconfnode_t *n;

  for(n = mdconf; n = mdconfsearch(n, ";output"); n = n->next) {
    output_t *o = output_create();
    o->next = outputs;
    outputs = o;

    o->target = target_parse(mdconfsearch(n, "target"));
    o->name = strdupnul(mdconfget(n, "name"));
    if(!o->name) o->name = strdupnul(n->value);
    o->region = strdupnul(mdconfget(n, "region"));
    o->file = strdupnul(mdconfget(n, "file"));
    if(!o->file) o->file = strdupnul(o->name);

    if(o->target == target_scd) {
      const section_t *section;
      section = sections;
      BLSLL_FINDSTR(section, name, "ip.asm");
      o->ip = section;

      section = sections;
      BLSLL_FINDSTR(section, name, "sp.asm");
      o->sp = section;
    } else {
      const symbol_t *entry = symbols;
      BLSLL_FINDSTR(entry, name, "MAIN");
      o->entry = entry;
    }

    o-> = strdupnul(mdconfget(n, ""));
  }
}
