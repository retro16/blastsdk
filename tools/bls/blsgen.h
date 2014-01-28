#ifndef BLS_BLSGEN_H
#define BLS_BLSGEN_H

#include "mdconf.h"

#define MDCONF_GET_INT(md, field, target, dflt) do { \
  const char *v; if((v = mdconfget(md, #field))) (target) = parse_int(v); else (target) = (dflt); \
while(0)

#define MDCONF_GET_STR(md, field, target, dflt) do { \
  const char *v; if((v = mdconfget(md, #field))) (target) = strdup(v); else (target) = strdup(dflt); \
while(0)

#define MDCONF_GET_ENUM(md, enumname, field, target, dflt) do { \
  const char *v; if((v = mdconfget(md, #field))) (target) = enumname ## _parse(v); else (target) = (dflt); \
while(0)

static inline char * strdupnul(const char *s) {
  if(!s)
    return 0;
  return strdup(s);
}

// A bus is an address space viewed from a CPU
typedef enum bus {
  bus_none,
  bus_main,
  bus_sub,
  bus_z80,
  bus_max
} bus_t;
BLSENUM(bus)

// A chip is a memory space
typedef enum chip {
  chip_none,
  chip_stack, // Pseudo-chip : push data onto stack.
  chip_cart,
  chip_bram, // Optional genesis in-cartridge battery RAM
  chip_zram,
  chip_vram,
  chip_ram,
  chip_pram,
  chip_wram,
  chip_pcm,
  chip_max
} chip_t;
BLSENUM(chip);

typedef struct busaddr {
  bus_t bus;
  sv_t addr;
  int bank; // -1 = unknown
} busaddr_t;

typedef struct chipaddr {
  chip_t chip;
  sv_t addr;
} chipaddr_t;

typedef enum {
  format_auto, // Guess from extension
  format_empty, // Empty space (do not store)
  format_zero, // zero-filled space (filled at runtime, no storage)
  format_raw, // Section ""
  format_asmx, // Section ""
  format_sdcc, // Sections .text .data .bss
  format_gcc, // Sections .text .data .bss
  format_as, // GNU AS. Sections .text .data .bss
  format_png // Sections .img .map .pal
} format_t;
BLSENUM(format);

struct section;
struct blsll_node_section_t;
typedef struct group {
  format_t format;
  char *name;
  int optimize; // Optimization level (used for compilation or compression level)

  const struct blsll_node_section_t *sections;
} group_t;
void group_free(group_t *group);
BLSLL_DECLARE(group_t, group_free);

struct section;
typedef struct symbol {
  char *name;
  chipaddr_t value;
  const struct section *section; // Section providing the symbol
} symbol_t;

struct blsll_node_section_t;
typedef struct section {
  char *name; // Segment name in source
  char *datafile; // Data file name

  sv_t physaddr; // Physical address on medium
  sv_t physsize; // Size on physical medium
  int physalign; // Alignment on physical medium

  const struct symbol *symbol; // Target address
  int align;
  sv_t size; // Size once loaded

  const BLSLL(symbol_t) *intsym; // Internal symbols
  const BLSLL(symbol_t) *extsym; // External symbols (dependencies)

  const struct blsll_node_section_t *deps; // Added by "uses", "provides" or symbol dependencies

  const group_t *source;
} section_t;
void section_free(section_t *section);
BLSLL_DECLARE(section_t, section_free);

void symbol_free(symbol_t *symbol);
BLSLL_DECLARE(symbol_t, symbol_free);


typedef enum target {
  target_gen,
  target_scd,
  target_vcart
} target_t;
BLSENUM(target_t)

typedef struct output {
  target_t target;
  char *name;
  char *region;
  char *file;
  const symbol_t *entry;
  const section_t *ip;
  const section_t *sp;

  const BLSLL(group_t) *binaries; // Binaries to put in the image
  const BLSLL(group_t) *bol; // Build order list
} output_t;
void output_free(output_t *output);
BLSLL_DECLARE(output_t, output_free);

#endif
