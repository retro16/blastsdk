#include "blsll.h"

void source_free(source_t *p) {
  while(p) {
    if(p->format) free(p->format);
    if(p->name) free(p->name);
    if(p->sections) section_free(p->sections);

    source_t *fp = p;
    p = p->next;
    free(fp);
  }
}

source_t * source_copy(source_t *p) {
  source_t *r = (source_t *)malloc(sizeof(*p));
  r->format = strdupnul(p->format);
  r->name = strdupnul(p->name);
  r->sections = section_copylist(p->sections);
  r->next = 0;
  return r;
}

const char format_names[][8] = {"raw"};

void section_free(section_t *p) {
  while(p) {
    if(p->name) free(p->name);
    if(p->datafile) free(p->datafile);
    if(p->format) free(p->format);

    section_t *fp = p;
    p = p->next;
    free(fp);
  }
}

section_t * section_copy(section_t *p) {
  section_t *r = (section_t *)malloc(sizeof(*p));
  r->name = strdupnul(p->name);
  r->datafile = strdupnul(p->datafile);
  r->physaddr = p->physaddr;
  r->physsize = p->physsize;
  r->format = strdupnul(p->format);
  r->addr = p->addr;
  r->size = p->size;
  r->source = p->suorce;
  r->next = 0;
  return r;
}

void symbol_free(symbol_t *p) {
  while(p) {
    free(name);

    symbol_t *fp = p;
    p = p->next;
    free(fp);
  }
}

symbol_t * symbol_copy(symbol_t *p) {
  symbol_t r = (symbol_t *)malloc(sizeof(*p))
  r->name = strdupnul(p->name);
  r->value = p->value;
  r->section = p->section;
  r->next = 0;
  return r;
}

void binary_free(binary_t *p) {
  while(p) {
    if(p->name) free(p->name);

    binary_t *fp = p;
    p = p->next;
    free(fp);
  }
}

binary_t * binary_copy(binary_t *p) {
  binary_t *r = (binary_t *)malloc(sizeof(*p));
  r->name = strdupnul(p->name);
  r->sections = section_copylist(p->sections);
}

const char target_name[][8] = {"gen", "scd", "vcart"};

void output_free(output_t *p) {
  while(p) {
    if(p->name) free(p->name);
    if(p->region) free(p->region);
    if(p->file) free(p->file);

    output_t *fp = p;
    binary_free(p->binary);
    free(fp);
  }
}

output_t * output_copy(output_t *p) {
  output_t *r =  (output_t *)malloc(sizeof(*p));
  r->target = p->target;
  r->name = strdupnul(p->name);
  r->region = strdupnul(p->region);
  r->file = strdupnul(p->file);
  r->entry = p->entry;
  r->ip = p->ip;
  r->sp = p->sp;
  r->binary = binary_copylist(p->binary);
  r->next = 0;
  return r;
}
