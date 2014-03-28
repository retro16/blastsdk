#ifndef BLS_BLSGEN_H
#define BLS_BLSGEN_H

#include <stdint.h>
#include <string.h>
#include "mdconf.h"
#include "blsll.h"


extern BLSLL(group) *sources;
extern BLSLL(section) *sections;
extern BLSLL(group) *binaries;
extern BLSLL(output) *outputs;
extern BLSLL(symbol) *symbols;


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

typedef int64_t sv;

// A bus is an address space viewed from a CPU
typedef enum bus {
  bus_none,
  bus_main,
  bus_sub,
  bus_z80,
  bus_max
} bus;
BLSENUM(bus, 8)

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
} chip;
BLSENUM(chip, 8)

typedef struct busaddr {
  bus bus;
  sv addr;
  int bank; // -1 = unknown
} busaddr;

typedef struct chipaddr {
  chip chip;
  sv addr;
} chipaddr;

typedef enum {
  format_auto, // Guess from extension
  format_empty, // Empty space (do not store)
  format_zero, // zero-filled space (filled at runtime, no storage)
  format_raw, // Section ""
  format_asmx, // Section ""
  format_sdcc, // Sections .text .data .bss
  format_gcc, // Sections .text .data .bss
  format_as, // GNU AS. Sections .text .data .bss
  format_png, // Sections .img .map .pal
  format_max
} format;
BLSENUM(format, 8)

// Sources are groups of sections
// Binaries are groups of sections

struct section;
struct blsll_node_section;
typedef struct group {
  format format;
  char *name;
  int optimize; // Optimization level (used for compilation or compression level)

  struct blsll_node_section *provides;
  struct blsll_node_section *uses;
} group;
group * group_new();
void group_free(group *group);
BLSLL_DECLARE(group, group_free)

struct section;
typedef struct symbol {
  char *name;
  chipaddr value;
  struct section *section; // Section providing the symbol
} symbol;
symbol * symbol_new();
void symbol_free(symbol *symbol);
BLSLL_DECLARE(symbol, symbol_free)

struct blsll_node_section;
typedef struct section {
  char *name; // Segment name in source: "", ".text", ".img", ".pal", ...
  char *datafile; // Data file name

  sv physaddr; // Physical address on medium
  sv physsize; // Size on physical medium
  int physalign; // Alignment on physical medium

  const struct symbol *symbol; // Target address
  int align;
  sv size; // Size once loaded

  BLSLL(symbol) *intsym; // Internal symbols
  BLSLL(symbol) *extsym; // External symbols (dependencies)

  struct blsll_node_section *deps; // Added by "uses", "provides" or symbol dependencies

  const group *source;
} section;
section * section_new();
void section_free(section *section);
BLSLL_DECLARE(section, section_free)


typedef enum target {
  target_gen,
  target_scd,
  target_vcart,
  target_max
} target;
BLSENUM(target, 8)

typedef struct output {
  target target;
  char *name;
  char *region;
  char *file;
  const symbol *entry;
  const section *ip;
  const section *sp;

  const BLSLL(group) *binaries; // Binaries to put in the image
  const BLSLL(group) *bol; // Build order list
} output;
void output_free(output *output);
BLSLL_DECLARE(output, output_free)

group * source_find(const char *name);
section * section_find(const char *name);
group * binary_find(const char *name);

#endif
