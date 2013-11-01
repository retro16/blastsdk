#include "bls.h"

#ifndef INCDIR
#define INCDIR "inc/"
#endif

u32 get_org(int srcid);
void gen_symbols(int srcid, bus_t bus, int ext); // Generate symbols includes for a given source. if ext is true, resolve external symbols

int compute_size_all(); // Size computation phase
void compile_all(); // Compilation phase
void link_all(); // Link phase

int compute_size_asm(int srcid); // Compile an assembler file (parse symbols and estimate size)
void compile_asm(int srcid);
void link_asm(int srcid); // Assemble an asm file

int compute_size_bin(int srcid);
void compile_bin(int srcid);
void link_bin(int srcid);

int compute_size_empty(int srcid);
void compile_empty(int srcid);
void link_empty(int srcid);

extern int compute_size_png(int srcid);
extern void compile_png(int srcid);
extern void link_png(int srcid);

extern int compute_size_pngpal(int srcid);
extern void compile_pngpal(int srcid);
extern void link_pngpal(int srcid);

int compute_size_c_m68k(int srcid);
void compile_c_m68k(int srcid);
void link_c_m68k(int srcid);

int compute_size_cxx_m68k(int srcid);
void compile_cxx_m68k(int srcid);
void link_cxx_m68k(int srcid);

int compute_size_gcc_m68k(int srcid, const char *compiler);
void compile_gcc_m68k(int srcid);
void link_gcc_m68k(int srcid);


void parse_lst(const char *file, int srcid, int setsym);
void parse_nm(int srcid, FILE* in);

u32 get_org(int srcid)
{
  src_t *s = &(src[srcid]);
  busaddr_t busaddr;
  busaddr = chip2bus(s->sym.value, s->bus);
  if(busaddr.addr == -1)
  {
    return 0;
  }
  return busaddr.addr;
}

void gen_symbols(int srcid, bus_t bus, int ext)
{
  char fname[1024];
  src_t *s = &(src[srcid]);

  snprintf(fname, 1024, TMPDIR"/%s.sym.inc", s->sym.name);
  FILE *af = fopen(fname, "w");

  snprintf(fname, 1024, TMPDIR"/%s.sym.h", s->sym.name);
  FILE *cf = fopen(fname, "w");

  snprintf(fname, 1024, TMPDIR"/%s.sym.sym", s->sym.name);
  FILE *sf = fopen(fname, "w");

  if(!af || !cf || !sf)
  {
    printf("Cannot create symbol file for %s.\n", s->sym.name);
    exit(1);
  }

#define OUTSYM(name, value) \
  fprintf(af, "%s\t\tequ\t$%08X\n", (name), (unsigned int)(value)); \
  fprintf(cf, "#define %s 0x%08X\n", (name), (unsigned int)(value)); \
  fprintf(sf, "%s = 0x%08X;\n", (name), (unsigned int)(value));

#define OUTSYMSUFFIX(name, suffix, value) \
  fprintf(af, "%s"suffix"\t\tequ\t$%08X\n", (name), (unsigned int)(value)); \
  fprintf(cf, "#define %s"suffix" 0x%08X\n", (name), (unsigned int)(value)); \
  fprintf(sf, "%s"suffix" = 0x%08X;\n", (name), (unsigned int)(value));

  OUTSYM("SCD", scd);
  OUTSYM("BUS", (int)bus);
  OUTSYM("PGMCHIP", (int)pgmchip);

  // Output internal symbols
  int i;
  int l = strlen(s->sym.name);
  fprintf(af, "; Internal symbols of %s\n", s->name);
  for(i = 0; i < SYMCNT; ++i)
  {
    if(s->intsym[i].name[0] != -1 && strncmp(s->intsym[i].name, s->sym.name, l) == 0)
    {
      sym_t *is = &s->intsym[i];
      busaddr_t busaddr = chip2bus(is->value, bus);
      fprintf(af, "%s\t\tequ\t$%08X\n", is->name, (u32)(busaddr.addr == -1 ? 0 : busaddr.addr));
      fprintf(cf, "#define %s 0x%08X\n", is->name, (u32)(busaddr.addr == -1 ? 0 : busaddr.addr));
      fprintf(sf, "%s = 0x%08X;\n", is->name, (u32)(busaddr.addr == -1 ? 0 : busaddr.addr));
    }
  }

  fprintf(af, "; External symbols of %s\n", s->name);
  // Output external symbols
  if(ext)
  {
    // Output dependencies
    int i;
    for(i = 0; i < SYMCNT; ++i)
    {
      const char *name = s->extsym[i];
      if(name[0] > 0 && name[0] != 'G' && name[1] != '_') // Filter-out global symbols
      {
        chipaddr_t val;
        if(!sym_get_value(&val, name))
        {
          printf("Warning: Symbol %s not found (needed by %s)\n", name, s->name);
          continue;
        }
        if(val.chip == chip_none || val.chip == chip_vram)
        {
          // Data value
          fprintf(af, "%s\t\tequ\t$%08X\n", name, (u32)(val.addr));
          fprintf(cf, "#define %s 0x%08X\n", name, (u32)(val.addr));
          fprintf(sf, "%s = 0x%08X;\n", name, (u32)(val.addr));

          fprintf(af, "%s_CHIP\t\tequ\t-1\n", name);
          fprintf(cf, "#define %s_CHIP -1\n", name);
          fprintf(sf, "%s_CHIP = -1;\n", name);
        }
        else
        {
          // Pointer value
          busaddr_t busaddr = chip2bus(val, bus);
          if(busaddr.bus == bus_none)
          {
            printf("Could not convert %s to bus address.\n", name);
            exit(1);
          }
          if(busaddr.bank >= 0)
          {
            fprintf(af, "%s_BANK\t\tequ\t%d\n", name,(u32)(busaddr.bank));
            fprintf(cf, "#define %s_BANK %d\n", name, (u32)(busaddr.bank));
            fprintf(sf, "%s_BANK = %d;\n", name, (u32)(busaddr.bank));
          }
          fprintf(af, "%s\t\tequ\t$%08X\n", name, (u32)(busaddr.addr));
          fprintf(cf, "#define %s 0x%08X\n", name, (u32)(busaddr.addr));
          fprintf(sf, "%s = 0x%08X;\n", name, (u32)(busaddr.addr));

          fprintf(af, "%s_CHIP\t\tequ\t%d\n", name,(u32)(val.chip));
          fprintf(cf, "#define %s_CHIP %d\n", name, (u32)(val.chip));
          fprintf(sf, "%s_CHIP = %d;\n", name, (u32)(val.chip));
        }
      }
    }

    // Output global symbols
    char glob[4096][NAMELEN];
    memset(glob, '\0', 4096*NAMELEN);

    int gsrc,globid;
    for(gsrc = 0; gsrc < SRCCNT; ++gsrc) if(gsrc != srcid)
    {
      src_t *gs = &src[gsrc];
      if(gs->name[0] <= 0) continue;

      fprintf(af, "; Globals set in %s\n", gs->name);
      for(i = 0; i < SYMCNT; ++i)
      {
        const char *name = gs->intsym[i].name;
        if(name[0] != 'G' || name[1] != '_' || strchr(name, '.')) continue;

        // Don't output self-defined globals
        if(sym_is_int(srcid, name))
          continue;

        // Avoid output duplicates 
        for(globid=0; globid < 4096; ++globid)
        {
          if(strcmp(glob[globid], name) == 0)
            goto global_already_output;
          
          if(!glob[globid][0])
          {
            strcpy(glob[globid], name);
            break;
          }
        }
        chipaddr_t val = gs->intsym[i].value;

        if(val.chip == chip_none || val.chip == chip_vram)
        {
          // Data value
          fprintf(af, "%s\t\tequ\t$%08X\n", name, (u32)(val.addr));
          fprintf(cf, "#define %s 0x%08X\n", name, (u32)(val.addr));
          fprintf(sf, "%s = 0x%08X;\n", name, (u32)(val.addr));
        }
        else
        {
          // Pointer value
          busaddr_t busaddr = chip2bus(val, bus);
          if(busaddr.bus == bus_none)
          {
            printf("Could not convert %s to bus address.\n", name);
            exit(1);
          }
          if(busaddr.bank >= 0)
          {
            fprintf(af, "%s_BANK\t\tequ\t%d\n", name,(u32)(busaddr.bank));
            fprintf(cf, "#define %s_BANK %d\n", name, (u32)(busaddr.bank));
            fprintf(sf, "%s_BANK = %d;\n", name, (u32)(busaddr.bank));
          }
          fprintf(af, "%s\t\tequ\t$%08X\n", name, (u32)(busaddr.addr));
          fprintf(cf, "#define %s 0x%08X\n", name, (u32)(busaddr.addr));
          fprintf(sf, "%s = 0x%08X;\n", name, (u32)(busaddr.addr));
        }
global_already_output:
        (void)0;
      }
    }
  }

#undef OUTSYM
#undef OUTSYMSUFFIX

  fclose(af);
  fclose(cf);
  fclose(sf);
}

int compute_size_all()
{
  int dirty = 0;
  int i;
  for(i = 0; i < SRCCNT; ++i)
  {
    if(src[i].bus == bus_none)
    {
      // Try to deduce bus from chip
      src[i].bus = find_bus(src[i].sym.value.chip);
      if(src[i].bus == bus_none)
      {
        printf("Could not find attached bus for source %s.\n", src[i].name);
        exit(1);
      }
    }
    ssize_t oldsize = src[i].binsize;
    switch(src[i].type)
    {
      default:
        break;
#define SRCTYPE(t) case type_ ## t: if(compute_size_ ## t(i)) {dirty = 1; src[i].dirty = 1;} break;
      SRCTYPE(asm);
      SRCTYPE(bin);
      SRCTYPE(empty);
      SRCTYPE(png);
      SRCTYPE(pngpal);
      SRCTYPE(c_m68k);
      SRCTYPE(cxx_m68k);
#undef SRCTYPE
    }
    if(oldsize != -1 && oldsize == src[i].binsize && src[i].dirty)
    {
      // In-place replacing in source
      src[i].dirty = 2;
    }
  }

  return dirty || !outdate || outdate < inidate;
}

void compile_all()
{
  int i;

  // Reset symbols
  for(i = 0; i < SRCCNT; ++i) if(src[i].type != type_none)
  {
    src_clear_sym(i);
    if(src[i].concataddr.chip != chip_none)
    {
      char n[NAMELEN];
      snprintf(n, NAMELEN, "%s_CONCAT", src[i].sym.name);
      sym_set_int_chip(i, n, src[i].concataddr);
    }
  }

  // Compile
  for(i = 0; i < SRCCNT; ++i)
  {
    if(src[i].dirty)
    {
      switch(src[i].type)
      {
        default:
          break;
#define SRCTYPE(t) case type_ ## t: compile_ ## t(i); break;
        SRCTYPE(asm);
        SRCTYPE(bin);
        SRCTYPE(empty);
        SRCTYPE(png);
        SRCTYPE(pngpal);
        SRCTYPE(c_m68k);
        SRCTYPE(cxx_m68k);
#undef SRCTYPE
      }
    }
  }
}

void link_all()
{
  int i;
  for(i = 0; i < SRCCNT; ++i)
  {
    if(src[i].dirty)
    {
      switch(src[i].type)
      {
        default:
          break;
#define SRCTYPE(t) case type_ ## t: link_ ## t(i); break;
        SRCTYPE(asm);
        SRCTYPE(bin);
        SRCTYPE(empty);
        SRCTYPE(png);
        SRCTYPE(pngpal);
        SRCTYPE(c_m68k);
        SRCTYPE(cxx_m68k);
#undef SRCTYPE
      }
    }
  }
}

int compute_size_asm(int srcid)
{
  src_t *s = &(src[srcid]);
  char cmdline[1024];

  sprintf(s->binname, TMPDIR"/%s.bin", s->sym.name);
/*  FIXME : this ignores dependencies
  if(fileexists(s->binname) && filedate(s->binname) > filedate(s->filename))
  {
    // Already compiled
    s->binsize = filesize(s->binname);
    return 0;
  }*/

  gen_symbols(srcid, s->bus, 0);

  // Compute command-line for first pass only
  snprintf(cmdline, 1024, "%s -1 -o %s -l "TMPDIR"/%s.1.lst -i "TMPDIR"/%s.sym.inc -I "STRINGIFY(INCDIR)" -C %s -b 0x%06x %s > /dev/null 2>&1", asmx, s->binname, s->sym.name, s->sym.name, s->bus == bus_z80 ? "z80" : "68k", get_org(srcid), s->filename);

//  printf("Getting symbols for %s :\n%s\n", s->name, cmdline);
  system(cmdline); // Call actual assembler
  unlink(s->binname);

  char buf[1024];
  snprintf(buf, 1024, TMPDIR"/%s.1.lst", s->sym.name);
  parse_lst(buf, srcid, 1);

  return 1;
}

void compile_asm(int srcid)
{
  src_t *s = &(src[srcid]);
  char cmdline[1024];

  gen_symbols(srcid, s->bus, 0);
  sprintf(s->binname, TMPDIR"/%s.bin", s->sym.name);

  // Compute command-line for first pass only
  snprintf(cmdline, 1024, "%s -1 -o %s -l "TMPDIR"/%s.2.lst -i "TMPDIR"/%s.sym.inc -I "STRINGIFY(INCDIR)" -C %s -b 0x%06x %s > /dev/null 2>&1", asmx, s->binname, s->sym.name, s->sym.name, s->bus == bus_z80 ? "z80" : "68k", get_org(srcid), s->filename);

  printf("Getting symbols for %s :\n%s\n", s->name, cmdline);
  system(cmdline); // Call actual assembler

  if(s->binsize < (int)filesize(s->binname))
  {
    printf("Error: compile pass assembly generated a bigger file (%06X => %06X).\n", (unsigned int)s->binsize, (unsigned int)filesize(s->binname));
    exit(1);
  }

  unlink(s->binname);

  // Store binsize because it may have been changed by address mapping
  int bs = s->binsize;

  char buf[1024];
  snprintf(buf, 1024, TMPDIR"/%s.2.lst", s->sym.name);
  parse_lst(buf, srcid, 1);
  s->binsize = bs;
}

void link_asm(int srcid)
{
  src_t *s = &(src[srcid]);

  char cmdline[1024];
  gen_symbols(srcid, s->bus, 1);

  int org = get_org(srcid);
  if(org == -1) {
    printf("Could not find ORG for source %s\n", s->name);
    exit(1);
  }

  // Compute command-line for actual assembly
  snprintf(cmdline, 1024, "%s -e -w -o %s -l "TMPDIR"/%s.lst -i "TMPDIR"/%s.sym.inc -I "STRINGIFY(INCDIR)" -C %s -b 0x%06x %s", asmx, s->binname, s->sym.name, s->sym.name, s->bus == bus_z80 ? "z80" : "68k", org, s->filename);

  printf("Compiling asm file %s :\n%s\n", s->name, cmdline);
  int res;
  if((res = system(cmdline)) != 0) // Call assembler
  {
    printf("Assembly failed for file %s : error code %d\n", s->name, res);
    exit(3);
  }

  char buf[1024];
  snprintf(buf, 1024, TMPDIR"/%s.lst", s->sym.name);
  parse_lst(buf, srcid, 1);

  if(s->binsize > (int)filesize(s->binname))
  {
    printf("Warning: link pass assembly generated a smaller file (%06X => %06X).\n", (unsigned int)s->binsize, (unsigned int)filesize(s->binname));
  }
  else if(s->binsize < (int)filesize(s->binname))
  {
    printf("Error: link pass assembly generated a bigger file (%06X => %06X).\n", (unsigned int)s->binsize, (unsigned int)filesize(s->binname));
    exit(1);
  }
}

int compute_size_bin(int srcid)
{
  src_t *s = &(src[srcid]);

  if(!fileexists(s->name))
  {
    printf("Error: Binary file %s not found.\n", s->name);
    exit(1);
  }

  s->binsize = filesize(s->name);

  sprintf(s->binname, TMPDIR"/%s.bin", s->sym.name);
  if(fileexists(s->binname) && filedate(s->binname) > filedate(s->filename))
  {
    // Already compiled
    return 0;
  }

  return 1;
}

void compile_bin(int srcid)
{
  // No need to compile
  (void)srcid;
}

void link_bin(int srcid)
{
  src_t *s = &(src[srcid]);

  char cmdline[1024];
  // Compute command-line for copy
  snprintf(cmdline, 1024, "cp %s %s", s->filename, s->binname);
  printf("Copying binary file %s :\n%s\n", s->filename, cmdline);
  int res;
  if((res = system(cmdline)) != 0) // Call assembler
  {
    printf("Copy failed for file %s : error code %d\n", s->filename, res);
    exit(3);
  }
}

int compute_size_empty(int srcid)
{
  src_t *s = &(src[srcid]);

  if(s->binsize == -1)
  {
    printf("Error: binsize must be declared for empty source %s.\n", s->name);
    exit(1);
  }

  sprintf(s->binname, TMPDIR"/%s.empty", s->sym.name);
  if(fileexists(s->binname) && filedate(s->binname) > inidate)
  {
    // Already compiled
    return 0;
  }

  return 1;

  // Already compiled
  return 0;
}

void compile_empty(int srcid)
{
  // No need to compile
  (void)srcid;
}

void link_empty(int srcid)
{
  src_t *s = &(src[srcid]);

  FILE *f = fopen(s->binname, "wb");
  if(!f)
  {
    printf("Cannot create cache file for BSS %s.\n", s->name);
    exit(1);
  }
  if(s->store)
  {
    // Write empty file
    ssize_t i;
    for(i = 0; i < s->binsize; ++i)
    {
      // Output all ones (required for ROM burning)
      fputc(255, f);
    }
  }
  else
  {
    // Write simple cache
    fwrite(&s->binsize, sizeof(s->binsize), 1, f);
  }
  fclose(f);
}

void parse_lst(const char *file, int srcid, int setsym)
{
  src_t *s = &src[srcid];
  FILE *f = fopen(file, "r");
  if(!f)
  {
    printf("Could not open lst file %s\n", file);
    exit(1);
  }

  while(!feof(f))
  {
    char line[4096];
    fgets(line, 4096, f);

    int l = strlen(line);

    if(l > 10 && s->sym.value.addr == -1 && line[0] >= '0')
    {
      const char *c = line;
      sv_t address = parse_hex(&c, 6);
      skipblanks(&c);
      if(strncasecmp("ORG", c, 3) == 0 && c[3] <= ' ')
      {
        // first ORG declaration : map to address
        busaddr_t ba;
        ba.bus = s->bus;
        ba.addr = address;
        ba.bank = -1;
        chipaddr_t ca = bus2chip(ba);
        if(s->sym.value.chip != chip_none && ca.chip != s->sym.value.chip)
        {
          printf("Error: ORG of source %s is not on the correct chip.\n", s->name);
          exit(1);
        }
        s->sym.value = ca;
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
      sv_t binend = parse_hex(&p, 8);
      s->binsize = binend - get_org(srcid);
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
        if(setsym)
        {
          busaddr_t busaddr = {s->bus, symval, s->bank};
          sym_set_int(srcid, sym, busaddr);
        }
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
          // Internal pointer
          if(!setsym)
            break;
          if(sym_is_ext(srcid, sym))
            break;
          {
            busaddr_t busaddr = {s->bus, symval, s->bank};
            sym_set_int(srcid, sym, busaddr);
          }
          break;
        case 'E':
        case 'S':
          // Internal symbol
          if(!setsym)
            break;
          if(sym_is_ext(srcid, sym))
            break;
          {
            busaddr_t busaddr = {bus_none, symval, -1};
            sym_set_int(srcid, sym, busaddr);
          }
          break;
        case 'U':
          sym_set_ext(srcid, sym);
          break;
        default:
          break;
      }
      ++c;
    }
  }
  fclose(f);
}

/* Compiling C :
 *
 * m68k-elf-gcc -Os -c -o test.o test.c
 * m68k-elf-ld -Ttext=org -R symtable -o test test.o
symtable:
m1 = 0xFF0000;
m2 = 0xFF0008;
 * m68k-elf-objcopy -O binary -j text test test.bin
 * m68k-elf-nm test
00002230 B __bss_start
00002230 B _edata
00002238 B _end
00002234 B m1
00002230 B m2
00000200 T main
         U _start
00000218 T toto
*/

int compute_size_gcc_m68k(int srcid, const char *compiler)
{
  src_t *s = &(src[srcid]);
  char cmdline[1024];
  char object[1024];
  char elf[1024];

  sprintf(s->binname, TMPDIR"/%s.bin", s->sym.name);
  sprintf(object, TMPDIR"/%s.o", s->sym.name);
  sprintf(elf, TMPDIR"/%s.elf", s->sym.name);

  gen_symbols(srcid, s->bus, 0);

  snprintf(cmdline, 1024, "%s %s -mcpu=68000 -c %s -o %s", compiler, cflags, s->filename, object);
  printf("First pass compilation of %s :\n%s\n", s->name, cmdline);
  system(cmdline);

  snprintf(cmdline, 1024, "%s %s -r -R "TMPDIR"/%s.sym.sym %s -o %s", ld, ldflags, s->sym.name, object, elf);
  printf("First pass linking of %s :\n%s\n", s->name, cmdline);
  system(cmdline);

  // Extract non-linked output
  snprintf(cmdline, 1024, "%s -O binary %s %s", objcopy, elf, s->binname);
  printf("Computing binary size of %s :\n%s\n", s->name, cmdline);
  system(cmdline);

  // Leave a small room because C linking may make the binary grow
  s->binsize = filesize(s->binname);
  s->binsize += 8 + s->binsize / 64;
	unlink(s->binname);
	unlink(elf);

	return 1;
}

void compile_gcc_m68k(int srcid)
{
  src_t *s = &(src[srcid]);
  char cmdline[1024];
  char object[1024];
  char elf[1024];
  u32 org = get_org(srcid);

  if(org == 0)
  {
    printf("Could not find ORG for source %s\n", s->name);
    exit(1);
  }

  sprintf(object, TMPDIR"/%s.o", s->sym.name);
  sprintf(elf, TMPDIR"/%s.elf", s->sym.name);

  gen_symbols(srcid, s->bus, 0);

  // Link to get internal symbols
  snprintf(cmdline, 1024, "%s %s -Ttext=0x%06X -r -R "TMPDIR"/%s.sym.sym %s -o %s", ld, ldflags, org, s->sym.name, object, elf);
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
  parse_nm(srcid, f);
  pclose(f);

  unlink(elf);
}

void link_gcc_m68k(int srcid)
{
  src_t *s = &(src[srcid]);
  char cmdline[1024];
  char object[1024];
  char elf[1024];
  u32 org = get_org(srcid);

  if(org == 0)
  {
    printf("Could not find ORG for source %s\n", s->name);
    exit(1);
  }

  sprintf(s->binname, TMPDIR"/%s.bin", s->sym.name);
  sprintf(object, TMPDIR"/%s.o", s->sym.name);
  sprintf(elf, TMPDIR"/%s.elf", s->sym.name);

  gen_symbols(srcid, s->bus, 1);

  // Link with all symbols defined
  snprintf(cmdline, 1024, "%s %s -nostdlib -Ttext=0x%06X -R "TMPDIR"/%s.sym.sym %s -o %s", ld, ldflags, org, s->sym.name, object, elf);
  printf("Linking binary %s :\n%s\n", s->name, cmdline);
  system(cmdline);

  // Extract the text section of elf to binary
  snprintf(cmdline, 1024, "%s -O binary %s %s", objcopy, elf, s->binname);
  printf("Extracting text section of %s :\n%s\n", s->name, cmdline);
  system(cmdline);

  if((size_t)filesize(s->binname) > (size_t)s->binsize)
  {
    printf("Error: Linking %s generated a bigger binary file : %08X -> %08X\n", s->name, (unsigned int)s->binsize, (unsigned int)filesize(s->binname));
    exit(1);
  }

  unlink(elf);
}

void parse_nm(int srcid, FILE* in)
{
  src_t *s = &(src[srcid]);
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
    int addr = parse_hex(&c, 8);

    skipblanks(&c); // Skip space
    char type = *(c++);
    skipblanks(&c);
    char symname[1024];
    parse_sym(symname, &c);

    switch(type)
    {
      case 'T':
      // Pointer in text section : associate to bus
        {
          busaddr_t busaddr = {s->bus, addr, s->bank};
          sym_set_int(srcid, symname, busaddr);
        }
      break;
      case 'U':
        sym_set_ext(srcid, symname);
      break;
      break;
      case 'b':
      case 'B':
      case 'd':
      case 'D':
        printf("Notice: source %s has a data/BSS symbol (%s)\n", s->name, symname);
      break;
    }
  }
}

int compute_size_c_m68k(int srcid)
{
  return compute_size_gcc_m68k(srcid, cc);
}

void compile_c_m68k(int srcid)
{
  compile_gcc_m68k(srcid);
}

void link_c_m68k(int srcid)
{
  link_gcc_m68k(srcid);
}

int compute_size_cxx_m68k(int srcid)
{
  return compute_size_gcc_m68k(srcid, cxx);
}

void compile_cxx_m68k(int srcid)
{
  compile_gcc_m68k(srcid);
}

void link_cxx_m68k(int srcid)
{
  link_gcc_m68k(srcid);
}
