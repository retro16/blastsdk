#ifndef BLS_BLSGEN_H
#define BLS_BLSGEN_H

#include "mdconf.h"

static inline char * strdupnul(const char *s) {
  if(!s)
    return 0;
  return strdup(s);
}

typedef enum {
  format_auto, // Guess from extension
  format_empty, // Empty space (do not store)
  format_bin, // Section ""
  format_asmx, // Section ""
  format_sdcc, // Sections .text .data .bss
  format_gcc, // Sections .text .data .bss
  format_as, // GNU AS. Sections .text .data .bss
  format_png // Sections .img .map .pal
} format_t;
BLSENUM(format);

struct section;
typedef struct source {
  format_t format;
  char *name;

  const struct section *sections;
} source_t;
void source_free(source_t *source);
BLSLL_DECLARE(source_t, source_free);

// Physical format
typedef enum {
  pfmt_ignore = 0, // Ignore data, leave as-is
  pfmt_zero = 1, // Zero-fill
  pfmt_raw = 2
} pfmt_t;
BLSENUM(pfmt);

struct symbol;
typedef struct section {
  char *name; // Segment name in source
  char *datafile; // Data file name

  sv_t physaddr; // Physical address on medium
  sv_t physsize; // Size on physical medium
  pfmt_t pfmt; // Format on physical storage

  struct symbol *symbol; // Target address
  sv_t size; // Size once loaded

  const source_t *source;
} section_t;
void section_free(section_t *section);
BLSLL_DECLARE(section_t, section_free);


typedef struct symbol {
  char *name;
  chipaddr_t value;
  const section_t *section;
} symbol_t;
void symbol_free(symbol_t *symbol);
BLSLL_DECLARE(symbol_t, symbol_free);


typedef struct binary {
  char *name;
  const section_t *sections;
} binary_t;
void binary_free(binary_t *binary);
BLSLL_DECLARE(binary_t, binary_free);


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

  const binary_t *binary;
} output_t;
void output_free(output_t *output);
BLSLL_DECLARE(output_t, output_free);

#endif
