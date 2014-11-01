#include "blsgen.h"
#include "blsconf.h"
#include "blsaddress.h"

#include <unistd.h>

/* blsgen functions to generate binaries from gcc sources */

void section_create_gcc(group *source, const mdconfnode *mdconf)
{
  (void)mdconf;
  section *s;

  if(source->banks.bus == bus_none && maintarget != target_scd1 && maintarget != target_scd2) {
    // For genesis, default to main bus
    source->banks.bus = bus_main;
  }

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
const char *cflags = " -Dextern='extern __attribute__((weak))' -fno-common -Os -pipe -fomit-frame-pointer -fno-builtin-memset -fno-builtin-memcpy";
const char *sflags = " -fno-common -pipe ";

const char *ld = "m68k-elf-ld";
const char *ldflags = "";

const char *nm = "m68k-elf-nm";

const char *readelf = "m68k-elf-readelf";
const char *objcopy = "m68k-elf-objcopy";
const char *objdump = "m68k-elf-objdump";


static void gen_symtable_section(FILE *out, const section *s)
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
    fprintf(out, "%s = 0x%08X;\n", sym->name, (uint32_t)addr);
    printf("%s = 0x%08X;\n", sym->name, (uint32_t)addr);
  }
}

static int is_asm(const void *elt)
{
  const element *s = (const element *)elt;

  if(strcasecmp(s->name + strlen(s->name) - 2, ".s") == 0) {
    return 1;
  }

  return 0;
}

static void gen_symtable(const group *s)
{
  section *text = section_find_ext(s->name, ".text");
  section *data = section_find_ext(s->name, ".data");
  section *bss = section_find_ext(s->name, ".bss");

  char symfile[4096];
  snprintf(symfile, 4096, BUILDDIR"/%s.sym", s->name);
  FILE *f = fopen(symfile, "w");

  fprintf(f, "BLAST_DUMMY_LINE_XXXXXXXXXX_ = 0x00000000;\n");

  gen_symtable_section(f, text);
  gen_symtable_section(f, data);
  gen_symtable_section(f, bss);

  fclose(f);
}

static section *extract_section_elf(const group *src, const char *name, unsigned int address)
{
  char linkerscript[4096];
  char cmdline[4096];

  section *s = section_find_ext(src->name, name);

  snprintf(linkerscript, 4096, BUILDDIR"/%s.ldscript", s->name);
  FILE *script = fopen(linkerscript, "w");

  switch(name[1]) {
  case 't':
    fprintf(script,
            "SECTIONS\n"
            "{\n"
            " .text 0x%08X :\n"
            " {\n"
            "  *(.text*)\n"
            "  *(.rodata*)\n"
            " }\n"
            " .other :\n"
            " {\n"
            "  *(.data)\n"
            "  *(.bss)\n"
            "  *(COMMON)\n"
            " }\n"
            "}\n"
            , address);
    break;

  case 'd':
    fprintf(script,
            "SECTIONS\n"
            "{\n"
            " .data 0x%08X :\n"
            " {\n"
            "  *(.data)\n"
            " }\n"
            " .other :\n"
            " {\n"
            "  *(.text*)\n"
            "  *(.rodata*)\n"
            "  *(.bss)\n"
            "  *(COMMON)\n"
            " }\n"
            "}\n"
            , address);
    break;

  case 'b':
    fprintf(script,
            "SECTIONS\n"
            "{\n"
            " .bss 0x%08X :\n"
            " {\n"
            "  *(.bss)\n"
            "  *(COMMON)\n"
            " }\n"
            " .other :\n"
            " {\n"
            "  *(.text*)\n"
            "  *(.rodata*)\n"
            "  *(.data)\n"
            " }\n"
            "}\n"
            , address);
    break;

  default:
    break;
  }

  fclose(script);

  snprintf(cmdline, 4096, "%s -r -T %s -o "BUILDDIR"/%s.elf "BUILDDIR"/%s.elf", ld, linkerscript, s->name, src->name);
  printf("Extracting section %s\n%s\n", s->name, cmdline);
  system(cmdline);

  return s;
}

void parse_symtable_dump(section *s, FILE *in, int setvalues)
{
  (void) setvalues;
  const char *sname = strrchr(s->name, '.');

  while(!feof(in)) {
    char line[1024];
    fgets(line, 1024, in);

    if(strlen(line) < 34) {
      continue;
    }

    // line : 00000ad4 g     O .data  00000004 myd

    sv addr = parse_hex(line);
    char symname[1024];
    const char *c;

    for(c = &line[17]; *c && *c != ' ' && *c != '\t'; ++c);

    parse_hex_skip(&c);
    skipblanks(&c);
    parse_sym(symname, &c);

    if(sname[0] == '.' && sname[1] == 't' && strncmp(line + 17, "*UND*", 5) == 0) {
      // Undefined (extern) symbol, referenced in text section
      symbol *sym;

      if((sym = symbol_find(symname))) {
        s->extsym = blsll_insert_unique_symbol(s->extsym, sym);
        printf("%s extern\n", symname);
      } else {
        symbol_def(&s->extsym, symname, NULL);
        printf("%s unknown\n", symname);
      }

      continue;
    }

    if(line[9] == 'l' || line[9] == ' ') {
      continue;
    }

    if(strncmp(line + 17, sname, strlen(sname)) != 0) {
      continue;
    }

    symbol_set_addr(&s->intsym, symname, addr, s);
    s->extsym = blsll_insert_unique_symbol(s->extsym, symbol_find(symname));
    printf("%s = %08X\n", symname, (unsigned int)addr);
  }
}

const char *binary_load_function = "BLSLOAD_BINARY_";

void binary_loaded(group *s, const char *symname)
{
  group *binary = binary_find_sym(symname);
  section *text = section_find_ext(s->name, ".text");

  if(binary) {
    text->loads = blsll_insert_group(text->loads, binary);
  } else {
    printf("Could not find binary with symbol name %s\n", symname);
    exit(1);
  }
}

void find_binary_load(group *s, FILE *in)
{
  while(!feof(in)) {
    char line[1024];
    fgets(line, 1024, in);

    const char *find = line;

    while(find && *find) {
      find = strstr(find, binary_load_function);

      if(find) {
        find += strlen(binary_load_function);
        char symname[1024];
        parse_sym(symname, &find);
        skipblanks(&find);

        if(find[0] != '(' || find[1] != ')') {
          continue;
        }

        ++find;

        binary_loaded(s, symname);
      }
    }
  }
}

static void extract_section(group *src, section *sec)
{
  const char *secname = strrchr(sec->name, '.');
  char cmdline[4096];
  snprintf(cmdline, 4096, "%s -O binary -j %s "BUILDDIR"/%s.elf "BUILDDIR"/%s", objcopy, secname, src->name, sec->name);
  printf("Extract section %s :\n%s\n", sec->name, cmdline);
  system(cmdline);
}

void gen_load_section_gcc(FILE *out, const section *sec, busaddr physba)
{
  busaddr ba = chip2bus(sec->symbol->value, bus_main);

  if(sec->format == format_empty || sec->size == 0) {
    return;
  }

  if(sec->format == format_zero) {
    if(sec->symbol->value.chip == chip_ram) {
      fprintf(out, "blsfastfill((void*)0x%08X, 0, 0x%08X);\n", (unsigned int)ba.addr, (unsigned int)sec->size);
    } else if(sec->symbol->value.chip == chip_vram) {
      fprintf(out, "blsvdp_dmafill_inline(0,(u16) 0x%04X, (u16)0x%04X);\n", (unsigned int)sec->symbol->value.addr, (unsigned int)sec->size);
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
    fprintf(out, "blsvdp_dma_inline(VDPVRAM, (u16)0x%04X, (void*)0x%08X, (u16)0x%04X);\n", (unsigned int)sec->symbol->value.addr, (unsigned int)physba.addr, (unsigned int)sec->size);
    break;

  case chip_cram:
    fprintf(out, "blsvdp_dma_inline(VDPCRAM, (u16)0x%04X, (void*)0x%08X, (u16)0x%04X);\n", (unsigned int)sec->symbol->value.addr, (unsigned int)physba.addr, (unsigned int)sec->size);
    break;

  case chip_wram:
  case chip_zram:
  case chip_ram:
    if(maintarget != target_ram && ba.addr != physba.addr) {
      fprintf(out, "blsfastcopy_aligned((void*)0x%08X, (void*)0x%08X, (u32)0x%08X);\n", (unsigned int)ba.addr, (unsigned int)physba.addr, (unsigned int)sec->size);
    }

    break;
  }
}

const char *gen_load_defines()
{
  static char filename[4096];
  snprintf(filename, 4096, BUILDDIR"/_blsload_defines.h");
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
    fprintf(out, "static inline void %s%s() { ", binary_load_function, binname);

    if(maintarget == target_scd1 || maintarget == target_scd2) {
      secl = bin->provides;
      if(bin->banks.bus == bus_main) {
        // Load from WRAM
        fprintf(out, "blsload_read_cd(0x%08X, 0x%04X);\n", (unsigned int)(bin->physaddr / CDBLOCKSIZE), (unsigned int)((bin->physsize + CDBLOCKSIZE - 1) / CDBLOCKSIZE));
        busaddr physba = {bus_main, 0x200000, -1};
        BLSLL_FOREACH(sec,secl) {
          if(bin == mainout.ipbin || bin == mainout.spbin) {
            // Layout for IP and SP is fixed
            physba.addr = 0x200000 + sec->symbol->value.addr;
          }

          // Load section from WRAM
          gen_load_section_gcc(out, sec, physba);

          // Compute offset of next section
          physba.addr += sec->size;
          if(physba.addr & 1) {
            ++physba.addr;
          }
        }
      } else {
        // Load from CD
        fprintf(out, "blsload_start_read(0x%08X, 0x%04X);\n", (unsigned int)(bin->physaddr / CDBLOCKSIZE), (unsigned int)((bin->physsize + CDBLOCKSIZE - 1) / CDBLOCKSIZE));
        BLSLL_FOREACH(sec,secl) {
          // Load from CD
          sv addr = chip2bank(sec->symbol->value, &sec->source->banks);
          fprintf(out, "blsload_read_cd(0x%08X, 0x%08X);\n", (unsigned int)addr, (unsigned int)sec->size);
        }
      }
    } else {
      secl = bin->provides;
      BLSLL_FOREACH(sec, secl) {
        // Load each section
        busaddr physba = phys2bus(sec->physaddr, bus_main);
        gen_load_section_gcc(out, sec, physba);
      }
    }

    fprintf(out, "}\n");
  }

  fclose(out);
  return filename;
}

static void section_get_size_gcc(section *s)
{
  // Generate and parse section sizes from elf binary
  char cmdline[4096];
  snprintf(cmdline, 4096, "%s -h "BUILDDIR"/%s.elf", objdump, s->name);
  printf("Extract section sizes from %s :\n%s\n", s->name, cmdline);
  FILE *in = popen(cmdline, "r");

  if(!in) {
    printf("Could not execute objdump on section %s\n", s->name);
    exit(1);
  }

  const char *sname = strrchr(s->name, '.');

  while(!feof(in)) {
    char line[1024];
    fgets(line, 1024, in);

    if(strlen(line) > 4 && strncmp(line + 4, sname, strlen(sname)) == 0) {
      s->size = parse_hex(line + 4 + strlen(sname));

      if(s->size == 1) {
        s->size = 0; // Workaround
        s->physsize = 0;
      }

      break;
    }
  }

  pclose(in);
}

static void parse_symbols(section *s, int setvalues)
{
  char cmdline[4096];
  snprintf(cmdline, 4096, "%s -t "BUILDDIR"/%s.elf", objdump, s->name);
  printf("Extract symbol%ss from %s :\n%s\n", setvalues ? " value" : "", s->name, cmdline);
  FILE *f = popen(cmdline, "r");

  if(!f) {
    printf("Could not execute objdump on source %s\n", s->name);
    exit(1);
  }

  parse_symtable_dump(s, f, setvalues);
  pclose(f);
  section_get_size_gcc(s);
}

void source_get_symbols_gcc(group *s)
{
  char cmdline[4096];
  char object[4096];
  char elf[4096];
  char srcname[4096];
  FILE *f;

  sprintf(object, BUILDDIR"/%s.o", s->name);
  sprintf(elf, BUILDDIR"/%s.elf", s->name);

  if(!findfile(srcname, s->file)) {
    printf("Error: %s not found\n", s->name);
    exit(1);
  }

  const char *defs = gen_load_defines();

  // Read binary loading from the source
  snprintf(cmdline, 4096, "%s %s -E -fdirectives-only %s", precompiler, include_prefixes, srcname);
  printf("Find binary loadings in %s :\n%s\n", s->name, cmdline);
  f = popen(cmdline, "r");

  if(!f) {
    printf("Could not execute cpp on source %s\n", s->name);
    exit(1);
  }

  find_binary_load(s, f);
  pclose(f);

  if(is_asm(s)) {
    snprintf(cmdline, 4096, "%s %s -DBUS=%d -DTARGET=%d -mcpu=68000 -c %s -o %s", compiler, include_prefixes, s->banks.bus, maintarget, srcname, object);
  } else {
    snprintf(cmdline, 4096, "%s %s %s -DBUS=%d -DTARGET=%d -include bls.h -include %s -mcpu=68000 -c %s -o %s", compiler, include_prefixes, cflags, s->banks.bus, maintarget, defs, srcname, object);
  }

  printf("First pass compilation of %s :\n%s\n", s->name, cmdline);
  system(cmdline);

  // Link to get internal symbols
  snprintf(cmdline, 4096, "%s %s -r %s -o %s", ld, ldflags, object, elf);
  printf("Get symbols from %s :\n%s\n", s->name, cmdline);
  system(cmdline);

  // Generate and parse symbols from elf binary
  parse_symbols(extract_section_elf(s, ".text", 0x40000), 0);
  parse_symbols(extract_section_elf(s, ".data", 0x440000), 0);
  parse_symbols(extract_section_elf(s, ".bss", 0x840000), 0);
}

void source_get_symbol_values_gcc(group *s)
{
  char cmdline[4096];
  char object[4096];
  char elf[4096];
  char srcname[4096];

  sprintf(object, BUILDDIR"/%s.o", s->name);
  sprintf(elf, BUILDDIR"/%s.elf", s->name);

  if(!findfile(srcname, s->file)) {
    printf("Error: %s not found\n", s->name);
    exit(1);
  }

  const char *defs = gen_load_defines();

  if(is_asm(s)) {
    snprintf(cmdline, 1024, "%s -DBUS=%d -DTARGET=%d -mcpu=68000 -c %s -o %s", compiler, s->banks.bus, maintarget, srcname, object);
  } else {
    snprintf(cmdline, 1024, "%s %s -DBUS=%d -DTARGET=%d -include bls.h -include %s %s -mcpu=68000 -c %s -o %s", compiler, include_prefixes, s->banks.bus, maintarget, defs, cflags, srcname, object);
  }

  printf("Compile %s with load defines :\n%s\n", s->name, cmdline);
  system(cmdline);

  section *text = section_find_ext(s->name, ".text");
  section *data = section_find_ext(s->name, ".data");
  section *bss = section_find_ext(s->name, ".bss");

  // Link to get internal symbols
  busaddr textba = chip2bus(text->symbol->value, s->banks.bus);
  busaddr databa = chip2bus(data->symbol->value, s->banks.bus);
  busaddr bssba = chip2bus(bss->symbol->value, s->banks.bus);
  snprintf(cmdline, 1024, "%s %s -r %s -o %s", ld, ldflags, object, elf);
  printf("Get internal symbol values from %s :\n%s\n", s->name, cmdline);
  system(cmdline);

  parse_symbols(extract_section_elf(s, ".text", (unsigned int)textba.addr), 1);
  parse_symbols(extract_section_elf(s, ".data", (unsigned int)databa.addr), 1);
  parse_symbols(extract_section_elf(s, ".bss", (unsigned int)bssba.addr), 1);
}

void source_compile_gcc(group *s)
{
  char cmdline[4096];
  char object[4096];
  char elf[4096];
  char srcname[4096];

  sprintf(object, BUILDDIR"/%s.o", s->name);
  sprintf(elf, BUILDDIR"/%s.elf", s->name);

  if(!findfile(srcname, s->file)) {
    printf("Error: %s not found\n", s->name);
    exit(1);
  }

  const char *defs = gen_load_defines();

  if(is_asm(s)) {
    snprintf(cmdline, 4096, "%s -DBUS=%d -DTARGET=%d -mcpu=68000 -c %s -o %s", compiler, s->banks.bus, maintarget, srcname, object);
  } else {
    snprintf(cmdline, 4096, "%s %s -DBUS=%d -DTARGET=%d -include bls.h -include %s %s -mcpu=68000 -c %s -o %s", compiler, include_prefixes, s->banks.bus, maintarget, defs, cflags, srcname, object);
  }

  printf("Final compilation of %s :\n%s\n", s->name, cmdline);
  system(cmdline);

  section *text = section_find_ext(s->name, ".text");
  section *data = section_find_ext(s->name, ".data");
  section *bss = section_find_ext(s->name, ".bss");

  gen_symtable(s);

  // Link at final address
  busaddr textba = chip2bus(text->symbol->value, s->banks.bus);
  busaddr databa = chip2bus(data->symbol->value, s->banks.bus);
  busaddr bssba = chip2bus(bss->symbol->value, s->banks.bus);
  snprintf(cmdline, 1024, "%s %s -Ttext=0x%08X -Tdata=0x%08X -Tbss=0x%08X -R "BUILDDIR"/%s.sym %s -o %s", ld, ldflags, (unsigned int)textba.addr, (unsigned int)databa.addr, (unsigned int)bssba.addr, s->name, object, elf);
printf("Link at final address %s :\n%s\n", s->name, cmdline);
system(cmdline);

// Extract sections from final ELF
extract_section_elf(s, ".text", (unsigned int)textba.addr);
extract_section_elf(s, ".data", (unsigned int)databa.addr);
extract_section_elf(s, ".bss", (unsigned int)bssba.addr);
extract_section(s, text);
extract_section(s, data);
extract_section(s, bss);
}

void source_premap_gcc(group *s)
{
section *text = section_find_ext(s->name, ".text");
section *data = section_find_ext(s->name, ".data");
section *bss = section_find_ext(s->name, ".bss");

printf(" !!!! premapping %s : %p;%p;%p\n", s->name, (void *)text, (void *)data, (void *)bss);

if(text->symbol->value.chip == chip_none) {
switch(s->banks.bus) {
case bus_none:
case bus_max:
break;

case bus_main:
if(maintarget == target_scd1 || maintarget == target_scd2) {
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

if(data->symbol->value.chip == chip_none) {
switch(s->banks.bus) {
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

if(bss->symbol->value.chip == chip_none) {
switch(s->banks.bus) {
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

// vim: ts=2 sw=2 sts=2 et
