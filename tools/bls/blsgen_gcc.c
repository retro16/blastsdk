#include "blsgen.h"
#include "blsconf.h"

#include <unistd.h>

/* blsgen functions to generate binaries from gcc sources */

void section_create_gcc(group *source, const mdconfnode *mdconf)
{
  (void)mdconf;
  section *s;

  // Generate the .text section
  source->provides = blsll_insert_section(source->provides, (s = section_parse_ext(NULL, source->name, ".text")));
  s->source = source;

  // Generate the .data section
  source->provides = blsll_insert_section(source->provides, (s = section_parse_ext(NULL, source->name, ".data")));
  s->source = source;

  // Generate the .bss section
  source->provides = blsll_insert_section(source->provides, (s = section_parse_ext(NULL, source->name, ".bss")));
  s->source = source;
  s->physsize = 0; // Do not store BSS on physical medium
  s->format = format_zero;
}

const char *compiler = "m68k-elf-gcc";
const char *precompiler = "m68k-elf-cpp";
const char *cflags = "-Os -pipe";

const char *ld = "m68k-elf-ld";
const char *ldflags = "";

const char *nm = "m68k-elf-nm";

const char *readelf = "m68k-elf-readelf";

void parse_nm(group *s, FILE *in, int setvalues)
{
  chipaddr unknown = {chip_none, -1};
  while(!feof(in))
  {
    char line[1024];
    fgets(line, 1024, in);

    printf("  %s", line);

    if(strlen(line) < 12)
    {
      continue;
    }

    const char *c = line;
    sv addr = parse_hex_skip(&c);

    skipblanks(&c); // Skip space
    char type = *(c++);
    skipblanks(&c);
    char symname[1024];
    parse_sym(symname, &c);

    section *text = section_find_ext(s->name, ".text");
    section *data = section_find_ext(s->name, ".data");
    section *bss = section_find_ext(s->name, ".bss");

    busaddr busaddr = {s->bus, addr, s->banks.bank[s->bus]};
    if(!setvalues) {
      busaddr.bus = bus_none;
      busaddr.addr = -1;
      busaddr.bank = -1;
    }

    switch(type)
    {
      case 'T':
      // Pointer in text section : associate to bus
        symbol_set_bus(&text->intsym, symname, busaddr, text);
        printf("%s = %08X  bus = %s\n", symname, (unsigned int)addr, bus_names[s->bus]);
        break;
      case 'U':
        symbol_set(&text->extsym, symname, unknown, text);
        printf("%s unknown\n", symname);
      break;
      case 'b':
      case 'B':
        symbol_set_bus(&bss->intsym, symname, busaddr, bss);
        printf("%s = %08X\n", symname, (unsigned int)addr);
        break;
      case 'd':
      case 'D':
        symbol_set_bus(&data->intsym, symname, busaddr, data);
        printf("%s = %08X\n", symname, (unsigned int)addr);
        break;
    }
  }
}

const char *binary_load_function = "BLS_LOAD_BINARY_";

void binary_loaded(group *s, const char *symname)
{
  group *binary = binary_find_sym(symname);
  if(binary) {
    s->loads = blsll_insert_group(s->loads, binary);
  } else {
    printf("Could not find binary with symbol name %s\n", symname);
    exit(1);
  }
}

void find_binary_load(group *s, FILE *in)
{
  while(!feof(in))
  {
    char line[1024];
    fgets(line, 1024, in);

    const char *find = line;
    while(find && *find)
    {
      find = strstr(find, binary_load_function);
      if(find) {
        find += strlen(binary_load_function);
        char symname[1024];
        parse_sym(symname, &find);
        skipblanks(&find);
        if(*find != ';') {
          continue;
        }
        ++find;

        binary_loaded(s, symname);
      }
    }
  }
}

const char * gen_load_defines(group *b)
{
  static char load[1024];

  load[0] = '\0';

  char *ptr = load;
  BLSLL(group) *lbl = b->loads;
  group *lb;

  BLSLL_FOREACH(lb, lbl) {
    char symname[1024];
    getsymname(symname, lb->name);
    ptr += snprintf(ptr, 1024 - (load - ptr), "-D%s%s ", binary_load_function, symname);
  }

  return load;
}

void source_get_size_gcc(group *s);
void source_get_symbols_gcc(group *s)
{
  char cmdline[1024];
  char object[1024];
  char elf[1024];
  FILE *f;

  sprintf(object, "%s.o", s->name);
  sprintf(elf, "%s.elf", s->name);

  // Read binary loading from the source
  snprintf(cmdline, 1024, "%s -E -fdirectives-only %s", precompiler, s->name);
  printf("Find binary loadings in %s :\n%s\n", s->name, cmdline);
  f = popen(cmdline, "r");
  if(!f)
  {
    printf("Could not execute cpp on source %s\n", s->name);
    exit(1);
  }
  find_binary_load(s, f);
  pclose(f);

  const char *defs = gen_load_defines(s);

  snprintf(cmdline, 1024, "%s %s %s -mcpu=68000 -c %s -o %s", compiler, defs, cflags, s->name, object);
  printf("First pass compilation of %s :\n%s\n", s->name, cmdline);
  system(cmdline);

  // Link to get internal symbols
  snprintf(cmdline, 1024, "%s %s -Ttext=0x40000 -r %s -o %s", ld, ldflags, object, elf);
  printf("Get symbols from %s :\n%s\n", s->name, cmdline);
  system(cmdline);

  // Generate and parse symbols from elf binary
  snprintf(cmdline, 1024, "%s %s", nm, elf);
  printf("Extract sybols from %s :\n%s\n", s->name, cmdline);
  f = popen(cmdline, "r");
  if(!f)
  {
    printf("Could not execute nm on source %s\n", s->name);
    exit(1);
  }
  parse_nm(s, f, 0);
  pclose(f);

  source_get_size_gcc(s);
}

void source_get_size_gcc(group *s)
{
  char cmdline[1024];
  char elf[1024];

  sprintf(elf, "%s.elf", s->name);

  // Generate and parse section sizes from elf binary
  snprintf(cmdline, 1024, "%s -t %s", readelf, elf);
  printf("Extract section sizes from %s :\n%s\n", s->name, cmdline);
  FILE *in = popen(cmdline, "r");
  if(!in)
  {
    printf("Could not execute readelf on source %s\n", s->name);
    exit(1);
  }

  char sectionname[256] = "";

  while(!feof(in))
  {
    char line[1024];
    fgets(line, 1024, in);
		if(*line) {
  		line[strlen(line) - 1] = '\0';
		}

    printf("  %s\n", line);

    if(strlen(line) < 8)
    {
      continue;
    }

    if(line[2] == '[') {
      strcpy(sectionname, line + 7);
    } else if(strlen(line) > 32 && line[31] == ' ') {
      sv size = parse_hex(line + 32);
      section *sec = section_find_ext(s->name, sectionname);
      if(!sec) {
        continue;
      }
      sec->size = size;
    }
  }

  pclose(in);
}

void source_premap_gcc(group *s)
{
  section *text = section_find_ext(s->name, ".text");
  section *data = section_find_ext(s->name, ".data");
  section *bss = section_find_ext(s->name, ".bss");

  if(text->symbol->value.chip == chip_none)
  {
    switch(s->bus) {
      case bus_none:
      case bus_max:
        break;
      case bus_main:
        if(mainout.target == target_scd) {
          text->symbol->value.chip = chip_ram;
        } else {
          text->symbol->value.chip = chip_cart;
        }
        break;
      case bus_sub:
        text->symbol->value.chip = chip_pram;
        break;
      case bus_z80:
        text->symbol->value.chip = chip_zram;
        break;
    }
  }

  if(data->symbol->value.chip == chip_none)
  {
    switch(s->bus) {
      case bus_none:
      case bus_max:
        break;
      case bus_main:
        data->symbol->value.chip = chip_ram;
        break;
      case bus_sub:
        data->symbol->value.chip = chip_pram;
        break;
      case bus_z80:
        data->symbol->value.chip = chip_zram;
        break;
    }
  }

  if(bss->symbol->value.chip == chip_none)
  {
    switch(s->bus) {
      case bus_none:
      case bus_max:
        break;
      case bus_main:
        bss->symbol->value.chip = chip_ram;
        break;
      case bus_sub:
        bss->symbol->value.chip = chip_pram;
        break;
      case bus_z80:
        bss->symbol->value.chip = chip_zram;
        break;
    }
  }
}
