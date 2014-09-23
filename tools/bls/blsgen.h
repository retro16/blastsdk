#ifndef BLS_BLSGEN_H
#define BLS_BLSGEN_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "bls.h"
#include "blsaddress.h"
#include "mdconf.h"

extern BLSLL(group) *sources;
extern BLSLL(section) *sections;
extern BLSLL(section) *usedsections; // Sections included in final image
extern BLSLL(group) *binaries;
extern BLSLL(group) *usedbinaries; // Binaries included in final image
extern BLSLL(output) *outputs;
extern BLSLL(symbol) *symbols;


#define MDCONF_GET_INT(md, field, target) do { \
  const char *v; if((v = mdconfget(md, #field))) (target) = parse_int(v); \
} while(0)
#define MDCONF_GET_INT_DFL(md, field, target, dflt) do { \
  const char *v; if((v = mdconfget(md, #field))) (target) = parse_int(v); else (target) = (dflt); \
} while(0)

#define MDCONF_GET_STR(md, field, target) do { \
  const char *v; if((v = mdconfget(md, #field))) (target) = strdup(v); \
} while(0)
#define MDCONF_GET_STR_DFL(md, field, target, dflt) do { \
  const char *v; if((v = mdconfget(md, #field))) (target) = strdup(v); else (target) = strdup(dflt); \
} while(0)

#define MDCONF_GET_ENUM(md, enumname, field, target) do { \
  const char *v; if((v = mdconfget(md, #field))) (target) = enumname ## _parse(v); \
} while(0)
#define MDCONF_GET_ENUM_DFL(md, enumname, field, target, dflt) do { \
  const char *v; if((v = mdconfget(md, #field))) (target) = enumname ## _parse(v); else (target) = (dflt); \
} while(0)


static inline char * strdupnul(const char *s) {
  if(!s)
    return 0;
  return strdup(s);
}

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
struct blsll_node_group;
typedef struct group {
  format format;
  char *name;
  sv physaddr; // Physical address on medium
  sv physsize; // Size on physical medium
  int physalign; // Alignment on physical medium

  int optimize; // Optimization level (used for compilation or compression level)
  bankconfig banks; // Current bus and status of banks when using the source

  struct blsll_node_section *provides;
  struct blsll_node_group *provides_sources;
  struct blsll_node_group *uses_binaries; // List of binaries used by this binary
  struct blsll_node_group *loads; // List of binaries loaded by this binary
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
  format format;
  char *name; // Source name with segment name appended
  sv physaddr; // Physical address on medium
  sv physsize; // Size on physical medium
  int physalign; // Alignment on physical medium

  char *datafile; // Data file name

  struct symbol *symbol; // Target address
  int align;
  sv size; // Size once loaded

  BLSLL(symbol) *intsym; // Internal symbols
  BLSLL(symbol) *extsym; // External symbols (dependencies)

  struct blsll_node_section *uses; // Added by "source.uses", "source.provides" or symbol dependencies
  struct blsll_node_group *loads; // List of binaries loaded by this section

  group *source;
} section;
section * section_new();
void section_free(section *section);
BLSLL_DECLARE(section, section_free)

typedef struct element {
  format format;
  char *name;
  sv physaddr; // Physical address on medium
  sv physsize; // Size on physical medium
  int physalign; // Alignment on physical medium

} element;
static inline void element_free(element *element) { (void)element; printf("Error: tried to free a generic element\n"); exit(1); }
BLSLL_DECLARE(element, element_free)

typedef struct output {
  target target;
  char *name;

  // ROM header information
  char *copyright;
  char *serial;
  sv rom_end; // Initially set to -1 to pad ROM to the NPOT or -2 to leave as-is
  int bram_type; // 0 = none, 1 = odd, 2 = even, 3 = both
  sv bram_start;
  sv bram_end;
  char *notes;
  char *region; // For SCD images, only one region is allowed

  char *file;
  symbol *entry; // Entry point for genesis
  group *entrybin; // Entry point binary for genesis
  section *ip;
  group *ipbin; // Binary of IP
  section *sp;
  group *spbin; // Binary of SP
  section *mainstack;
  section *substack;

  BLSLL(group) *bol; // Build order list
  BLSLL(group) *sol; // Section order lost
  BLSLL(group) *binaries; // Binaries to put in the image (in order)
} output;
output * output_new();
void output_free(output *output);
BLSLL_DECLARE(output, output_free)

extern output mainout;

group * source_find(const char *name);
group * source_find_sym(const char *name);
section * section_find(const char *name);
section * section_find_ext(const char *name, const char *suffix);
section * section_find_sym(const char *name);
section * section_find_using(section *section);
symbol * symbol_find(const char *name);
sv symbol_get_addr(const char *name, bankconfig *bankconfig);
chip symbol_get_chip(const char *name);
group * binary_find(const char *name);
group * binary_find_sym(const char *name);
group * binary_find_providing(BLSLL(group) * glist, section *section);

int sections_overlap(section *s1, section *s2);
int phys_overlap(element *s1, element *s2);

symbol * symbol_def(BLSLL(symbol) **symlist, char *symname, section *section);
symbol * symbol_set(BLSLL(symbol) **symlist, char *symname, chipaddr value, section *section);
symbol * symbol_set_bus(BLSLL(symbol) **symlist, char *symname, busaddr value, section *section);
symbol * symbol_set_addr(BLSLL(symbol) **symlist, char *symname, sv value, section *section);

void *merge_lists(void *target, void *source);

extern char path_prefixes[][4096];
extern char include_prefixes[4096];

void skipblanks(const char **cp);
sv parse_int_skip(const char **cp);
sv parse_int(const char *cp);
sv parse_hex_skip(const char **cp);
sv parse_hex(const char *cp);
void parse_sym(char *s, const char **cp);
size_t getsymname(char *output, const char *path);

int findfile(char *out, const char *f);
void tmpdir(char *out, const char *f); // Returns the file name in the temporary directory

#endif
