#include "blsll.h"

void source_free(source_t *p) {
  if(p->format) free(p->format);
  if(p->name) free(p->name);
  free(p);
}

const char format_names[][8] = {"raw"};

void section_free(section_t *p) {
  if(p->name) free(p->name);
  if(p->datafile) free(p->datafile);
  if(p->format) free(p->format);
  free(p);
}

void symbol_free(symbol_t *p) {
  free(name);
  free(p);
}

void binary_free(binary_t *p) {
  if(p->name) free(p->name);
  free(p);
}

const char target_name[][8] = {"gen", "scd", "vcart"};

void output_free(output_t *p) {
  if(p->name) free(p->name);
  if(p->region) free(p->region);
  if(p->file) free(p->file);
  free(p);
}
