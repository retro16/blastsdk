#include "blsgen.h"
#include "blsconf.h"
#include "blsaddress.h"

#include <unistd.h>

/* blsgen functions to generate binaries from gcc sources */

void section_create_asmx(group *source, const mdconfnode *mdconf)
{
  (void)mdconf;
  section *s;

  source->provides = blsll_insert_unique_section(source->provides, (s = section_parse_ext(mdconf, source->name, ".bin")));
  s->source = source;

  if(strstr(s->name, "_ram.asm.bin")) {
    if(s->symbol->value.chip == chip_none) {
      s->symbol->value.chip = chip_ram;
    }

    if(s->format == format_auto) {
      s->format = format_empty;
    }

    if(source->banks.bus == bus_none) {
      source->banks.bus = bus_main;
    }
  } else if(strstr(s->name, "_cart.asm.bin")) {
    if(s->symbol->value.chip == chip_none) {
      s->symbol->value.chip = chip_cart;
    }

    if(source->banks.bus == bus_none) {
      source->banks.bus = bus_main;
    }
  } else if(strstr(s->name, "_pram.asm.bin")) {
    if(s->symbol->value.chip == chip_none) {
      s->symbol->value.chip = chip_pram;
    }

    if(s->format == format_auto) {
      s->format = format_empty;
    }

    if(source->banks.bus == bus_none) {
      source->banks.bus = bus_sub;
    }
  } else if(strstr(s->name, "_wram.asm.bin")) {
    if(s->symbol->value.chip == chip_none) {
      s->symbol->value.chip = chip_wram;
    }

    if(source->banks.bus == bus_none) {
      source->banks.bus = bus_sub;
    }
  } else if(source->banks.bus == bus_none && maintarget != target_scd1 && maintarget != target_scd2) {
    // For genesis, default to main bus
    source->banks.bus = bus_main;
  }
}

static void gen_symtable_section_asmx(FILE *out, const section *s)
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

    sv addr = chip2bank(sym->value, &s->source->banks);
    fprintf(out, "%s\tEQU\t$%08X;\n", sym->name, (uint32_t)addr);
  }
}

static void gen_symtable_asmx(const group *s)
{
  section *sec = section_find_ext(s->name, ".bin");

  char symfile[4096];
  snprintf(symfile, 4096, BUILDDIR"/%s.sym", s->name);
  FILE *f = fopen(symfile, "w");

  gen_symtable_section_asmx(f, sec);

  fclose(f);
}

static void parse_lst_asmx(group *src, FILE *f, int setvalues)
{
  printf("####### parse_lst_asm %s %d\n", src->name, setvalues);
  section *sec = section_find_ext(src->name, ".bin");

  while(!feof(f)) {
    char line[4096];
    fgets(line, 4096, f);

    int l = strlen(line);

    if(l > 10 && sec->symbol->value.addr == -1 && line[0] >= '0') {
      const char *c = line;
      sv address = parse_hex_skip(&c);
      skipblanks(&c);

      if(strncasecmp("ORG", c, 3) == 0 && c[3] <= ' ') {
        // first ORG declaration : map to address
        if(src->banks.bus == bus_none) {
          // Guess bus based on context
          if((maintarget == target_scd1 || maintarget == target_scd2) && address < 0xA00000) {
            src->banks.bus = bus_sub;
          } else {
            src->banks.bus = bus_main;
          }
        }

        busaddr ba;
        ba.bus = src->banks.bus;
        ba.addr = address;
        ba.bank = -1;
        chipaddr ca = bus2chip(ba);

        if(sec->symbol->value.chip != chip_none && ca.chip != sec->symbol->value.chip) {
          printf("Error: ORG (%08X) of source %s (bus=%s) is not on the correct chip. Source is on chip %s and ORG is in %s\n", (unsigned int)address, src->name, bus_names[ba.bus], chip_names[sec->symbol->value.chip], chip_names[ca.chip]);
          exit(1);
        }

        sec->symbol->value = ca;
        printf("Set address from ORG : 0x%06X\n", (unsigned int)address);
        sec->fixed = 2;
      }
    }

    const char *sub;

    if(l > 10
        && ((line[0] >= '0' && line[0] <= '9') || (line[0] >= 'A' && line[0] <= 'F'))
        && (sub = strstr(line, "BLSLOAD_BINARY_"))) {
      sub += 15;
      char binname[4096];
      parse_sym(binname, &sub);

      group *binary = binary_find_sym(binname);

      if(binary) {
        printf("asm %s loads %s\n", src->name, binary->name);
        sec->loads = blsll_insert_unique_group(sec->loads, binary);
      } else {
        printf("Could not find binary with symbol name %s\n", binname);
        exit(1);
      }
    }

    if(line[0] >= '0' && line[0] <= '9' && l >= 20 && strcmp(&line[5], " Total Error(s)\n") == 0) {
      break;
    }
  }

  // After error count : catch symbols and ending address
  while(!feof(f)) {
    char line[4096];
    char sym[256];
    fgets(line, 4096, f);
    int l = strlen(line);

    if(l < 8) {
      continue;
    }

    if(strcmp(&line[8], " ending address\n") == 0) {
      // Parse ending address to find out estimated size
      // Only appears in one-pass compilation phase
      sv binend = parse_hex(line);

      if(sec->symbol->value.addr != -1) {
        sv newsize = binend - chip2bank(sec->symbol->value, &src->banks);
        if(newsize != sec->size) {
          printf("Warning: %s changed size (%06X -> %06X)\n", sec->name, (u32)sec->size, (u32)newsize);
          sec->size = newsize;
        }
      } else {
        sec->size = binend - 0x40000;
        printf("## SIZE = $%04X\n", (unsigned int)sec->size);
      }

      continue;
    }

    const char *c = line;

    while(*c) {
      // Skip blanks
      skipblanks(&c);
      // Parse symbol name
      parse_sym(sym, &c);
      // Skip blanks to find value
      skipblanks(&c);

      if(*c < '0' || *c > 'F') {
        break;
      }

      sv symval = parse_hex_skip(&c);

      if(*c == '\n' || !*c) {
        if(!setvalues) {
          symbol_def(&sec->intsym, sym, sec);
        } else {
          symbol_set_addr(&sec->intsym, sym, symval, sec);
        }

        continue;
      }

      if(*c != ' ') {
        printf("Expected space instead of [%02X] in source listing of %s\n[%s]\n", *c, src->name, c);
        exit(1);
      }

      ++c;

      switch(*c) {
      case ' ':
      case '\n':
      case '\r':
      case 'E':
      case 'S': {
        // Internal pointer
        symbol *ts;

        if((ts = symbol_find(sym)) && ts->section != sec && ts->section != NULL) {
          break; // Do not touch external symbols
        }

        if(!setvalues) {
          symbol_def(&sec->intsym, sym, sec);
        } else {
          symbol_set_addr(&sec->intsym, sym, symval, sec);
        }

        break;
      }

      case 'U': {
        symbol *ts;

        if((ts = symbol_find(sym))) {
          sec->extsym = blsll_insert_symbol(sec->extsym, ts);
          printf("%s extern: %s %08X\n", sym, chip_names[ts->value.chip], ts->value.addr);
        } else {
          symbol_def(&sec->extsym, sym, NULL);
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

void parse_symbols_asmx(group *src, int setvalues)
{
  char filename[4096];
  snprintf(filename, 4096, BUILDDIR"/%s.lst", src->name);
  FILE *f = fopen(filename, "r");
  parse_lst_asmx(src, f, setvalues);
  fclose(f);
}

void gen_load_section_asmx(FILE *out, const section *sec, busaddr physba)
{
  busaddr ba = chip2bus(sec->symbol->value, bus_main);

  if(sec->format == format_empty || sec->size == 0) {
    return;
  }

  if(sec->format == format_zero) {
    if(sec->symbol->value.chip == chip_ram) {
      fprintf(out, "\tBLSFASTFILL\t$%08X, 0, $%08X\n", (unsigned int)ba.addr, (unsigned int)sec->size);
    } else if(sec->symbol->value.chip == chip_vram) {
    fprintf(out, "\tVDPSETAUTOINCR 1\n");
      fprintf(out, "\tVDPDMAFILL\t0, $%04X, $%04X\n", (unsigned int)sec->symbol->value.addr, (unsigned int)sec->size);
    } else {
      printf("Warning : chip %s does not support format_zero\n", chip_names[sec->symbol->value.chip]);
    }

    return;
  }

  if(sec->format != format_raw) {
    printf("Warning : %s format unsupported\n", format_names[sec->format]);
    return;
  }

  switch(sec->symbol->value.chip) {
  case chip_none:
  case chip_mstk:
  case chip_sstk:
  case chip_zstk:
  case chip_cart:
  case chip_bram:
  case chip_pram:
  case chip_pcm:
  case chip_max:
    break;

  case chip_vram:
    fprintf(out, "\tVDPSETAUTOINCR 2\n");
    fprintf(out, "\tVDPSEND\t$%08X, $%04X, $%04X, VRAM\n", (unsigned int)physba.addr, (unsigned int)sec->symbol->value.addr, (unsigned int)sec->size);
    break;

  case chip_cram:
    fprintf(out, "\tVDPSETAUTOINCR 2\n");
    fprintf(out, "\tVDPSEND\t$%08X, $%04X, $%04X, CRAM\n", (unsigned int)physba.addr, (unsigned int)sec->symbol->value.addr, (unsigned int)sec->size);
    break;

  case chip_wram:
  case chip_zram:
  case chip_ram:
    if(maintarget != target_ram && ba.addr != physba.addr) {
      fprintf(out, "\tBLSFASTCOPY_ALIGNED\t$%08X, $%08X, $%08X\n", (unsigned int)ba.addr, (unsigned int)physba.addr, (unsigned int)sec->size);
    }

    break;
  }
}

const char *gen_load_defines_asmx()
{
  static char filename[4096];
  snprintf(filename, 4096, BUILDDIR"/_blsload_defines.asm");
  FILE *out = fopen(filename, "w");

  if(!out) {
    printf("Error : cannot open %s for writing.\n", filename);
    exit(1);
  }

  BLSLL(symbol) *syml;
  symbol *sym;
  BLSLL(section) *secl;
  section *sec;
  BLSLL(group) *binl;
  group *bin;

  FILE *defout = fopen(BUILDDIR"/_blsgen_defines.asm", "w");
  syml = mainout.globals;
  BLSLL_FOREACH(sym, syml) {
    fprintf(defout, "%s\tSET\t$%08X\n", sym->name, (u32)sym->value.addr);
  }
  fclose(defout);

  binl = binaries;
  BLSLL_FOREACH(bin, binl) {
    char binname[1024];
    getsymname(binname, bin->name);
    fprintf(out, "BLSLOAD_BINARY_%s\tMACRO\n", binname);

    if(maintarget == target_scd1 || maintarget == target_scd2) {
      secl = bin->provides;
      if(bin->banks.bus == bus_main) {
        // Load from WRAM
        fprintf(out, "\tBLSLOAD_READ_CD\t$%08X, $%04X\n", (unsigned int)(bin->physaddr / CDBLOCKSIZE), (unsigned int)((bin->physsize + CDBLOCKSIZE - 1) / CDBLOCKSIZE));
        busaddr physba = {bus_main, 0x200000, -1};
        BLSLL_FOREACH(sec,secl) {
          if(bin == mainout.ipbin || bin == mainout.spbin) {
            // Layout for IP and SP is fixed
            physba.addr = 0x200000 + sec->symbol->value.addr;
          }

          // Load section from WRAM
          gen_load_section_asmx(out, sec, physba);

          // Compute offset of next section
          physba.addr += sec->size;
          if(physba.addr & 1) {
            ++physba.addr;
          }
        }
      } else {
        // Load from CD
        fprintf(out, "\tBLSLOAD_START_READ\t$%08X, $%04X\n", (unsigned int)(bin->physaddr / CDBLOCKSIZE), (unsigned int)((bin->physsize + CDBLOCKSIZE - 1) / CDBLOCKSIZE));
        BLSLL_FOREACH(sec,secl) {
          // Load from CD
          sv addr = chip2bank(sec->symbol->value, &sec->source->banks);
          fprintf(out, "\tBLSLOAD_READ_CD\t$%08X, $%08X\n", (unsigned int)addr, (unsigned int)sec->size);
        }
      }
    } else {
      secl = bin->provides;
      BLSLL_FOREACH(sec, secl) {
        // Load each section
        busaddr physba = phys2bus(sec->physaddr, bus_main);
        gen_load_section_asmx(out, sec, physba);
      }
    }

    fprintf(out, "\tENDM\n");
  }

  fclose(out);
  return filename;
}

void source_get_symbols_asmx(group *s)
{
  char srcname[4096];
  char cmdline[4096];

  const char *defs = gen_load_defines_asmx();

  if(!findfile(srcname, s->file)) {
    printf("Error: %s not found (source %s)\n", s->file, s->name);
    exit(1);
  }

  section *sec = section_find_ext(s->name, ".bin");
  sv org = 0x40000;

  if(s->banks.bus != bus_none && sec && sec->symbol && sec->symbol->value.addr != -1) {
    org = chip2bank(sec->symbol->value, &s->banks);
  }

  snprintf(cmdline, 4096, "asmx -C 68000 -b 0x%06X -w -e -1 %s -i "BUILDDIR"/_blsgen_defines.asm -i bls.inc -i %s -d CHIP:=%d -d BUS:=%d -d SCD:=%d -d TARGET:=%d -l " BUILDDIR "/%s.lst -o /dev/null %s", (unsigned int)org, include_prefixes, defs, sec->symbol->value.chip, s->banks.bus, maintarget, maintarget, s->name, srcname);
printf("First pass compilation of %s :\n%s\n", s->name, cmdline);
system(cmdline);
snprintf(cmdline, 4096, "cp "BUILDDIR"/%s.lst "BUILDDIR"/%s.lst.1", s->name, s->name);
system(cmdline);

// Generate and parse symbols from listing
parse_symbols_asmx(s, 0);
}

void source_compile_asmx(group *s)
{
char srcname[4096];
char cmdline[4096];

const char *defs = gen_load_defines_asmx();
gen_symtable_asmx(s);

if(!findfile(srcname, s->file)) {
printf("Error: %s not found (source %s)\n", s->file, s->name);
exit(1);
}

section *sec = section_find_ext(s->name, ".bin");
if(sec->symbol->value.addr == -1 || sec->size == 0) {
  return;
}
sv org = chip2bank(sec->symbol->value, &s->banks);

snprintf(cmdline, 4096, "asmx -C 68000 -b 0x%06X -w -e %s -i "BUILDDIR"/_blsgen_defines.asm -i bls.inc -i %s -d CHIP:=%d -d BUS:=%d -d SCD:=%d -d TARGET:=%d -i "BUILDDIR"/%s.sym -l "BUILDDIR"/%s.lst -o "BUILDDIR"/%s.bin %s", (unsigned int)org, include_prefixes, defs, sec->symbol->value.chip, s->banks.bus, maintarget, maintarget, s->name, s->name, s->name, srcname);
printf("\n\nSecond pass compilation of %s :\n%s\n", s->name, cmdline);
system(cmdline);

// Generate and parse symbols from listing
parse_symbols_asmx(s, 1);

}

void source_get_symbol_values_asmx(group *s)
{
source_compile_asmx(s);
}

void source_premap_asmx(group *src)
{
section *sec = section_find_ext(src->name, ".bin");

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
