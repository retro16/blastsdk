#include "bls.h"

char output[256];
char author[256];
char title[256];
char masterdate[16]; // MMDDYYYY format
char region[4]; // 'J', 'U' or 'E' or any combination. Only the first is used for scd.
char copyright[17];
char serial[17]; // Serial number
int scd; // 0 = no SCD, 1 = SCD mode 1M, 2 = SCD mode 2M  (mode may change during runtime)
chip_t pgmchip;

time_t inidate;
time_t outdate;

FILE *imgout;

// Global boot sources. To be replaced by real source entries
int ipsrc;
int spsrc;

bus_t parse_bus(const char *s);
chip_t parse_chip(const char *s);
type_t parse_type(const char *s);
int parse_addr(const char *s);
void parse_ini(const char *file);


bus_t parse_bus(const char *s)
{
#define BUSNAME(n) if(strcmp(s, #n) == 0) return bus_ ## n
  BUSNAME(none);
  BUSNAME(main);
  BUSNAME(sub);
  BUSNAME(z80);
#undef BUSNAME
  return bus_none;
}

chip_t parse_chip(const char *s)
{
#define CHIPNAME(n) if(strcmp(s, #n) == 0) return chip_ ## n
  CHIPNAME(none);
  CHIPNAME(cart);
  CHIPNAME(bram);
  CHIPNAME(zram);
  CHIPNAME(vram);
  CHIPNAME(ram);
  CHIPNAME(pram);
  CHIPNAME(wram);
  CHIPNAME(pcm);
#undef CHIPNAME
  return chip_none;
}

type_t parse_type(const char *s)
{
#define TYPE(t) if(strcmp(s, #t) == 0) return type_ ## t
  TYPE(empty);
  TYPE(bin);
  TYPE(asm);
  TYPE(c_m68k);
  TYPE(cxx_m68k);
  TYPE(as);
  TYPE(c_z80);
  TYPE(png);
  TYPE(pngspr);
  TYPE(pngpal);
#undef TYPE
  printf("Invalid type %s\n", s);
  exit(1);
  return type_none;
}


int parse_addr(const char *s)
{
  return parse_int(&s, 6);
}

void parse_ini(const char *file)
{
  int i;

  // Set default global values
  strcpy(output, "blsbuild.bin");
  *author = 0;
  *title = 0;
  *masterdate = 0;
  region[0]= 'J';
  region[1] = 'U';
  region[2] = 'E';
  region[3] = 0;
  *copyright = 0;
  *serial = 0;
  scd = 0;
  pgmchip = chip_cart;
  for(i = 0; i < SRCCNT; ++i)
  {
    src[i].type = type_none;
  }
  for(i = 0; i < GRPCNT; ++i)
  {
    groups[i][0] = '\0';
  }

  // Read file

  FILE *f = fopen(file, "r");
  if(!f)
  {
    printf("Could not open source list file %s\n", file);
    exit(1);
  }

  inidate = filedate(file);

  int s = -1;
  while(!feof(f))
  {
    int llen;
    char line[4096];
    fgets(line, 4096, f);
    llen = strnlen(line, 4096);
    if(llen >= 4096) {
      printf("Warning : line too long, ignored\n");
      continue;
    }
    
    // Skip comment lines
    if(line[0] == '\0' || line[0] == '#' || line[0] == ';' || line[0] == '/' || line[0] == '\r' || line[0] == '\n')
    {
      continue;
    }

    // Strip end of line marker
    while(line[llen-1] == '\n' || line[llen-1] == '\r') {
      line[llen-1] = '\0';
      --llen;
    }

    if(s < 0) {
#define FIELD(t,n) if(strncmp(#n "=", line, strlen(#n "=")) == 0) { n = parse_ ## t(&line[strlen(#n "=")]); } else
#define STRFIELD(n,l) if(strncmp(#n "=", line, strlen(#n "=")) == 0) { strncpy(n, (&line[strlen(#n "=")]), l); } else
#define HEXFIELD(n) if(strncmp(#n "=", line, strlen(#n "=")) == 0) { const char *x=(&line[strlen(#n "=")]); n=parse_hex(&x, 8); } else
#define INTFIELD(n) if(strncmp(#n "=", line, strlen(#n "=")) == 0) { const char *x=(&line[strlen(#n "=")]); n=parse_int(&x, 8); } else
      STRFIELD(cc, 1024)
      STRFIELD(cxx, 1024)
      STRFIELD(cflags, 1024)
      STRFIELD(as, 1024)
      STRFIELD(asflags, 1024)
      STRFIELD(ld, 1024)
      STRFIELD(ldflags, 1024)
      STRFIELD(objcopy, 1024)
      STRFIELD(nm, 1024)
      STRFIELD(asmx, 1024)

      STRFIELD(output, 256)
      STRFIELD(author, 256)
      STRFIELD(title, 256)
      STRFIELD(masterdate, 16)
      STRFIELD(region, 3)
      STRFIELD(copyright, 16)
      STRFIELD(serial, 16)
      INTFIELD(scd)
      FIELD(chip, pgmchip)
      if(line[0] != '[') { printf("Invalid header parameter : %s\n", line); }
#undef FIELD
#undef STRFIELD
#undef HEXFIELD
#undef INTFIELD
    }
    if(line[0] == '[') {
      // Copy tag content
      char name[NAMELEN];
      char *l = &line[1];
      char *n = name;
      while(*l != ']' && (n - name) < NAMELEN) {
        *n = *l;
         ++l;
         ++n;
      }
      *n = '\0';
      for(s = 0; s < SRCCNT; ++s)
      {
        if(src[s].type == type_none)
        {
          // Add a new source
          src_clear(s);
          strncpy(src[s].name, name, NAMELEN);
          strncpy(src[s].filename, name, NAMELEN);
          getsymname(src[s].sym.name, name);
          char ext[64];
          getext(ext, name);
          if(strcasecmp(name, "asm") == 0) src[s].type = type_asm;
          else if(strcasecmp(name, "c") == 0) src[s].type = type_c_m68k;
          else if(strcasecmp(name, "cxx") == 0) src[s].type = type_cxx_m68k;
          else if(strcasecmp(name, "s") == 0) src[s].type = type_as;
          else if(strcasecmp(name, "png") == 0) src[s].type = type_png;
          else if(strcasecmp(name, "bin") == 0) src[s].type = type_bin;
          break;
        }
      }
    } else if(s >= 0) {
#define FIELD(n) if(strncmp(#n "=", line, strlen(#n "=")) == 0) { src[s].n = parse_ ## n(&line[strlen(#n "=")]); } else
#define STRFIELD(n) if(strncmp(#n "=", line, strlen(#n "=")) == 0) { strcpy(src[s].n, &line[strlen(#n "=")]); } else
#define INTFIELD(n) if(strncmp(#n "=", line, strlen(#n "=")) == 0) { const char *v=&line[strlen(#n "=")]; src[s].n = parse_int(&v, 10); } else
#define HEXFIELD(n) if(strncmp(#n "=", line, strlen(#n "=")) == 0) { const char *v=&line[strlen(#n "=")]; src[s].n = parse_hex(&v, 8); } else
#define SUBFIELD(n,st) if(strncmp(#n "=", line, strlen(#n "=")) == 0) { src[s].st = parse_ ## n(&line[strlen(#n "=")]); } else
      STRFIELD(filename)
      STRFIELD(concat)
      FIELD(type)
      FIELD(bus)
      INTFIELD(bank)
      INTFIELD(align)
      INTFIELD(store)
      INTFIELD(binsize)
      SUBFIELD(chip,sym.value.chip)
      SUBFIELD(addr,sym.value.addr)
      FIELD(groups)
      INTFIELD(physaddr)
      INTFIELD(physalign)
#undef FIELD
#undef STRFIELD
#undef INTFIELD
#undef HEXFIELD
#undef SUBFIELD

      { printf("Invalid parameter in %s : %s\n", src[s].name, line); }
    }
  }

  fclose(f);

  if(fileexists(output))
  {
    outdate = filedate(output);
  }
  else
  {
    outdate = 0;
  }
}
