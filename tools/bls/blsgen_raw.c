#include "blsgen.h"
#include "blsconf.h"
#include "blsfile.h"
#include "blsaddress.h"

#include <unistd.h>

/* blsgen functions to generate binaries from gcc sources */

void section_create_raw(group *source, const mdconfnode *mdconf)
{
  (void)mdconf;
  section *s;

  source->provides = blsll_insert_unique_section(source->provides, (s = section_parse_ext(mdconf, source->name, "")));
  s->source = source;

  if(source->banks.bus == bus_none && maintarget != target_scd1 && maintarget != target_scd2) {
    // For genesis, default to main bus
    source->banks.bus = bus_main;
  }

  printf("Source %s create RAW\n", source->name);
}

void source_get_symbols_raw(group *s)
{
  char srcname[4096];
  if(!findfile(srcname, s->file)) {
    printf("Error: %s not found (source %s)\n", s->file, s->name);
    exit(1);
  }

  section *sec = section_find_ext(s->name, "");
  sec->size = filesize(srcname);
}

void source_compile_raw(group *s)
{
  char srcname[4096];
  if(!findfile(srcname, s->file)) {
    printf("Error: %s not found (source %s)\n", s->file, s->name);
    exit(1);
  }

  char binname[4096];
  snprintf(binname, sizeof(binname), BUILDDIR"/%s", s->name);
  filecopy(srcname, binname);

  section *sec = section_find_ext(s->name, "");
  sec->size = filesize(srcname);

  printf("SOURCE COMPILE RAW. size = 0x%08X\n", (u32)sec->size);
}

void source_premap_raw(group *src)
{
  section *sec = section_find_ext(src->name, "");

  if(sec->symbol->value.chip == chip_none) {
    switch(src->banks.bus) {
      case bus_none:
      case bus_max:
      break;

      case bus_main:
      if(maintarget == target_scd1 || maintarget == target_scd2) {
        sec->symbol->value.chip = chip_ram;
      } else {
        sec->symbol->value.chip = chip_cart;
      }
      break;

      case bus_sub:
      sec->symbol->value.chip = chip_pram;
      break;

      case bus_z80:
      sec->symbol->value.chip = chip_zram;
      break;
    }
  }

  printf("%s addr %06X\n", sec->name, (unsigned int)sec->symbol->value.addr);
}

// vim: ts=2 sw=2 sts=2 et
