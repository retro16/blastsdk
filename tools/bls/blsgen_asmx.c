#include "blsgen.h"
#include "blsconf.h"
#include "blsaddress.h"

#include <unistd.h>

/* blsgen functions to generate binaries from gcc sources */

void section_create_asmx(group *source, const mdconfnode *mdconf)
{
  (void)mdconf;
  section *s;

  source->provides = blsll_insert_section(source->provides, (s = section_parse_ext(NULL, source->name, ".bin")));
  s->source = source;

  if(source->bus == bus_none && mainout.target != target_scd) {
    // For genesis, default to main bus
    source->bus = bus_main;
  }
}

static void gen_symtable_section(FILE *out, const section *s, bus bus)
{
  const BLSLL(symbol) *syml = s->extsym;
  const symbol *sym;
  BLSLL_FOREACH(sym, syml) {
    if(!sym->name) {
      printf("Warning : unnamed symbol needed by %s\n", s->name);
      continue;
    }
    if(sym->value.addr < 0) {
      printf("Warning : Undefined symbol needed by %s : %s\n", s->name, sym->name);
      continue;
    }
    busaddr ba = chip2bus(sym->value, bus);
    fprintf(out, "%s\tEQU\t$%08X;\n", sym->name, (uint32_t)ba.addr);
  }
}

static void gen_symtable(const group *s)
{
  section *sec = section_find_ext(s->name, ".bin");

  char symfile[4096];
  snprintf(symfile, 4096, BUILDDIR"/%s.sym", s->name);
  FILE *f = fopen(symfile, "w");

  gen_symtable_section(f, sec, s->bus);

  fclose(f);
}

static void parse_lst(section *s, FILE *f, int setvalues)
{
  while(!feof(f))
  {
    char line[4096];
    fgets(line, 4096, f);

    int l = strlen(line);

    if(l > 10 && s->symbol->value.addr == -1 && line[0] >= '0')
    {
      const char *c = line;
      sv address = parse_hex(&c, 6);
      skipblanks(&c);
      if(strncasecmp("ORG", c, 3) == 0 && c[3] <= ' ')
      {
        // first ORG declaration : map to address
        busaddr_t ba;
        ba.bus = s->bus;
        ba.addr = address;
        ba.bank = -1;
        chipaddr ca = bus2chip(ba);
        if(s->symbol->value.chip != chip_none && ca.chip != s->symbol->value.chip)
        {
          printf("Error: ORG of source %s is not on the correct chip.\n", s->name);
          exit(1);
        }
        s->symbol->value = ca;
      }
    }

		char *sub;
		if(l > 10 && (sub = strstr("Illegal opcode 'BLS_LOAD_BINARY_")))
		{
			sub += 33;
			char end = sub;
			while(*end && *end != '\'') ++end;
			char binname = (char*)alloca(end - sub + 1);
			memcpy(binname, sub, end - sub);
			binname[end - sub] = '\0';

			group *binary = binary_find_sym(binname);
			if(binary) {
				s->loads = blsll_insert_unique_group(s->loads, binary);
			} else {
				printf("Could not find binary with symbol name %s\n", binname);
				exit(1);
			}
		}

    if(line[0] >= '0' && line[0] <= '9' && l >= 20 && strcmp(&line[5], " Total Error(s)\n") == 0)
    {
      break;
    }
  }

  // After error count : catch symbols and ending address
  while(!feof(f))
  {
    char line[4096];
    char sym[256];
    fgets(line, 4096, f);
    int l = strlen(line);
    if(l < 8)
    {
      continue;
    }
    if(strcmp(&line[8], " ending address\n") == 0)
    {
      // Parse ending address to find out estimated size
      // Only appears in one-pass compilation phase
      const char *p = line;
      sv binend = parse_hex(&p);
      s->binsize = binend - chip2bus(s->symbol->value).addr;
      continue;
    }

    const char *c = line;
    while(*c)
    {
      // Skip blanks
      skipblanks(&c);
      // Parse symbol name
      parse_sym(sym, &c);
      // Skip blanks to find value
      skipblanks(&c);
      if(*c < '0' || *c > 'F')
      {
        break;
      }
      sv_t symval = parse_hex(&c, 8);
      if(*c == '\n' || !*c)
      {
				busaddr_t busaddr = {s->source->bus, symval, s->source->banks.bank[s->source->bus]};
        if(!setvalues)
        {
					busaddr.bus = bus_none;
					busaddr.addr = -1;
					busaddr.bank = -1;
				}
				symbol_set_bus(&s->intsym, symname, busaddr, s);
        continue;
      }
      if(*c != ' ')
      {
        printf("Expected space instead of [%02X] in file %s\n[%s]\n", *c, file, c);
        exit(1);
      }
      ++c;
      switch(*c)
      {
        case ' ':
        case '\n':
        case '\r':
        case 'E':
        case 'S':
				{
          // Internal pointer
					symbol *ts;
					if((ts = symbol_find(sym)) && ts->section != s)
					{
					  break; // Do not touch external symbols
					}
					busaddr_t busaddr = {s->source->bus, symval, s->source->banks.bank[s->source->bus]};
					if(!setvalues)
					{
						busaddr.bus = bus_none;
						busaddr.addr = -1;
						busaddr.bank = -1;
					}
					symbol_set_bus(&s->intsym, sym, busaddr, s);
          break;
				}
        case 'U':
				{
					symbol *ts;
					if((ts = symbol_find(sym)))
					{
						s->extsym = blsll_insert_symbol(s->extsym, sym);
						printf("%s extern\n", sym);
					}
					else
					{
						symbol_set(&s->extsym, sym, unknown, NULL);
						printf("%s unknown\n", sym);
					}
          break;
				}
        default:
          break;
      }
      ++c;
    }
  }
}

void parse_symbols(section *s, int setvalues)
{
	char filename[4096];
	snprintf(filename, 4096, BUILDDIR"/%s.lst", s->name);
	FILE *f = fopen(filename, "r");
	parse_lst(s, f, setvalues);
	fclose(f);
}

const char *gen_load_defines()
{
  static char filename[4096];
  snprintf(filename, 4096, BUILDDIR"/_blsload_defines.asm");
  FILE *out = fopen(filename, "w");

  if(!out) {
    printf("Error : cannot open %s for writing.\n", filename);
    exit(1);
  }

  BLSLL(section) *secl;
  section *sec;
  BLSLL(group) *binl;
  group *bin;

  binl = binaries;
  BLSLL_FOREACH(bin, binl) {
    char binname[1024];
    getsymname(binname, bin->name);
    fprintf(out, "BLS_LOAD_BINARY_%s\tMACRO", binname);
    if(mainout.target == target_scd) {
      // Begin SCD transfer
      // blsload_scd_stream starts CD transfer and waits until data is ready in hardware buffer
//      fprintf(out, "blsload_scd_stream(0x%08X, %d);", bin->physaddr / 2048, (bin->physsize + 2047) / 2048);
    } else {
      secl = bin->provides;
      BLSLL_FOREACH(sec, secl) {
        // Load each section
        busaddr ba = chip2bus(sec->symbol->value, bus_main);
        if(sec->format == format_empty || sec->size == 0) continue;
        if(sec->format == format_zero) {
          if(sec->symbol->value.chip == chip_ram) {
            if(sec->size > 4) {
              fprintf(out, "\t\tBLSFASTFILL_WORD\t$%08X, $%08X, 0\n", (unsigned int)ba.addr, (unsigned int)sec->size / 2);
            } else if(sec->size == 4) {
              fprintf(out, "\t\tCLR.L\t$%08X\n", (unsigned int)(ba.addr + sec->size - 1));
						} else if(sec->size >= 2) {
              fprintf(out, "\t\tCLR.W\t$%08X\n", (unsigned int)(ba.addr + sec->size - 1));
						}
            if(sec->size & 1) {
              fprintf(out, "\t\tCLR.B\t$%08X\n", (unsigned int)(ba.addr + sec->size - 1));
            }
          } else if(sec->symbol->value.chip == chip_vram) {
            fprintf(out, "\t\tBLSVDP_CLEAR\t0x%04X, 0x%04X\n", (unsigned int)sec->symbol->value.addr, (unsigned int)sec->size);
          } else {
            printf("Warning : chip %s does not support format_zero\n", chip_names[sec->symbol->value.chip]);
          }
          continue;
        }
        if(sec->format != format_raw) {
          printf("Warning : %s format unsupported\n", format_names[sec->format]);
          continue;
        }
        switch(sec->symbol->value.chip) {
        case chip_none:
        case chip_mstack:
        case chip_sstack:
        case chip_zstack:
        case chip_cart:
        case chip_bram:
        case chip_pram:
        case chip_wram:
        case chip_pcm:
        case chip_max:
          break;

        case chip_zram:
          // TODO
          break;

        case chip_vram:
          fprintf(out, "VDPDMASEND\t#%08X, #%04X, #%04X, VRAM\n", (unsigned int)sec->physaddr, (unsigned int)sec->symbol->value.addr, (unsigned int)sec->size);
          break;
        case chip_cram:
          fprintf(out, "VDPDMASEND\t#%08X, #%04X, #%04X, CRAM\n", (unsigned int)sec->physaddr, (unsigned int)sec->symbol->value.addr, (unsigned int)sec->size);
          break;
        case chip_ram:
          fprintf(out, "BLSFASTCOPY_WORD\t#%08X, #%08X, #%08X\n", (unsigned int)ba.addr, (unsigned int)sec->physaddr, (unsigned int)((sec->size + 1) / 2));
          break;
        }
      }
    }
    fprintf(out, "\t\tENDM\n");
  }

  fclose(out);
  return filename;
}

void source_get_symbols_asmx(group *s)
{
  char srcname[4096];
  FILE *f;

  const char *defs = gen_load_defines();

  snprintf(cmdline, 4096, "asmx -C 68000 -w -e -1 %s -i %s -l "BUILDDIR"/%s.lst -o "BUILDDIR"/%s.bin %s", include_prefixes, defs, s->name, s->name, srcname);
  printf("First pass compilation of %s :\n%s\n", s->name, cmdline);
  system(cmdline);

  // Generate and parse symbols from listing
  parse_symbols(s, 0);
}

void source_compile_asmx(group *s)
{
  char srcname[4096];
  FILE *f;

  const char *defs = gen_load_defines();

  snprintf(cmdline, 4096, "asmx -C 68000 -w -e -1 %s -i %s -l "BUILDDIR"/%s.lst -o "BUILDDIR"/%s.bin %s", include_prefixes, defs, s->name, s->name, srcname);
  printf("First pass compilation of %s :\n%s\n", s->name, cmdline);
  system(cmdline);

  // Generate and parse symbols from listing
  parse_symbols(s, 1);
}

void source_get_symbol_values_asmx(group *s)
{
	source_compile_asmx(s);
}

void source_premap_asmx(group *src)
{
  section *sec = section_find_ext(src->name, ".bin");

  if(sec->symbol->value.chip == chip_none) {
    switch(src->bus) {
    case bus_none:
    case bus_max:
      break;
    case bus_main:
      if(mainout.target == target_scd) {
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
}
