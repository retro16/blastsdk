#ifndef BLS_BLSGEN_H
#define BLS_BLSGEN_H

char * strdupnul(const char *s) {
  if(!s)
    return 0;
  return strdup(s);
}

struct section;

typedef struct source {
  char *format;
  char *name;

  struct section *sections;
  struct source *next;
} source_t;
BLSLL(source);

typedef enum {
  fmt_raw = 0
} format_t;
BLSENUM(format);

typedef struct section {
  char *name; // Segment name in source
  char *datafile; // Data file name

  sv_t physaddr; // Physical address on medium
  sv_t physsize; // Size on physical medium
  format_t format; // Runtime loading routine suffix

  chipaddr_t addr; // Target address
  sv_t size; // Size once loaded

  const source_t *source;

  struct section *next;
} section_t;
BLSLL(section);


typedef struct symbol {
  char *name;
  chipaddr_t value;
  const section_t *section;
  struct symbol *next;
} symbol_t;
BLSLL(symbol);


typedef struct binary {
  char *name;
  section_t *sections;

  struct binary *next;
} binary_t;
BLSLL(binary);


typedef enum target {
  target_gen,
  target_scd,
  target_vcart
} target_t;
BLSENUM(target)

typedef struct output {
  target_t target;
  char *name;
  char *region;
  char *file;
  const symbol_t *entry;
  const section_t *ip;
  const section_t *sp;

  binary_t *binary;
  struct output *next;
} output_t;
BLSLL(output);

#endif
