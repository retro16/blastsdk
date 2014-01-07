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

  for(n = mdconf->child; (n = mdconfsearch(n, "source")); n = n->next) {
    source_t *s = source_create();
    s->next = sources;
    sources = s;

    s->name = strdupnul(mdconfget(n->child, "name"));
    if(!s->name) s->name = strdupnul(n->value);
    s->format = format_parse(mdconfget(n->child, "format"));
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
        else s->format = format_bin;
      }
      else s->format = format_bin;
    }
    
    switch(s->format) {
      case format_bin:
        section_create_bin(s);
        break;
      case format_asmx:
        section_create_asmx(s);
        break;
      case format_gcc:
        section_create_gcc(s);
        break;
      case format_as:
        section_create_as(s);
        break;
      case format_png:
        section_create_png(s);
        break;
    }
  }

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