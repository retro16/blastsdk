#include "blsgen.h"
#include "blsconf.h"

#include <unistd.h>

/* blsgen functions to generate binaries from gcc sources */

void section_create_gcc(group *source, const mdconfnode *mdconf)
{
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
const char *cflags = "-Os -pipe";

const char *ld = "m68k-elf-ld";
const char *ldflags = "";

const char *nm = "m68k-elf-nm";

const char *readelf = "m68k-elf-readelf";

void parse_nm(group *s, FILE *in)
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

    switch(type)
    {
      case 'T':
      // Pointer in text section : associate to bus
        symbol_set_bus(&text->intsym, symname, busaddr, text);
        printf("%s = %08X  bus = %s\n", symname, addr, bus_names[s->bus]);
        break;
      case 'U':
        symbol_set(&text->extsym, symname, unknown, text);
        printf("%s unknown\n", symname, addr);
      break;
      case 'b':
      case 'B':
        symbol_set_bus(&bss->intsym, symname, busaddr, bss);
        printf("%s = %08X\n", symname, addr);
        break;
      case 'd':
      case 'D':
        symbol_set_bus(&data->intsym, symname, busaddr, data);
        printf("%s = %08X\n", symname, addr);
        break;
    }
  }
}
void source_get_size_gcc(group *s);
void source_get_symbols_gcc(group *s)
{
  char cmdline[1024];
  char object[1024];
  char elf[1024];

  sprintf(object, "%s.o", s->name);
  sprintf(elf, "%s.elf", s->name);

  snprintf(cmdline, 1024, "%s %s -mcpu=68000 -c %s -o %s", compiler, cflags, s->name, object);
  printf("First pass compilation of %s :\n%s\n", s->name, cmdline);
  system(cmdline);

  // Link to get internal symbols
  snprintf(cmdline, 1024, "%s %s -Ttext=0x40000 -r %s -o %s", ld, ldflags, object, elf);
  printf("Get symbols from %s :\n%s\n", s->name, cmdline);
  system(cmdline);

  // Generate and parse symbols from elf binary
  snprintf(cmdline, 1024, "%s %s", nm, elf);
  printf("Extract sybols from %s :\n%s\n", s->name, cmdline);
  FILE *f = popen(cmdline, "r");
  if(!f)
  {
    printf("Could not execute nm on source %s\n", s->name);
    exit(1);
  }
  parse_nm(s, f);
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

    const char *c = line;
  }

  pclose(in);
}
