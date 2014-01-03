#include "blsll.h"

typedef struct source {
  char *format;
  char *file;

  struct source *next;
} source_t;
BLSLL(source);


typedef struct segment {
  char *file; // Data file name

  sv_t physaddr; // Physical address on medium
  sv_t physsize; // Size on physical medium
  char *loader; // Runtime loading routine

  chipaddr_t addr; // Target address
  sv_t size; // Size once loaded

  const source_t *source;

  struct segment *next;
} segment_t;
BLSLL(segment);


typedef struct binary {
  char *name;
  segment_t *segments;

  struct binary *next;
} binary_t;
BLSLL(binary);


typedef enum target {
  TGT_GEN,
  TGT_SCD,
  TGT_VCART
} target_t;

typedef struct output {
  target_t target;
  char *name;
  char *region;
  char *file;
  char *entry;
  char *subentry;
  char *ip;
  char *sp;

  binary_t *binary;
  struct output *next;
} output_t;
BLSLL(output);

