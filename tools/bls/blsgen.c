#include "blsll.h"

void source_free(source_t *p) {
  if(p->format) free(p->format);
   if(p->name) free(p->name);
  free(p);
}

const char bus_names[][8] = {"none", "main", "sub", "z80"};
const char chip_names[][8] = {"none", "stack", "cart", "bram", "zram", "vram", "ram", "pram", "wram", "pcm"};
const char format_names[][8] = {"auto", "empty", "zero", "raw", "asmx", "sdcc", "gcc", "as", "png"};

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

int main() {
  blsconf_load("blsgen.md");

}
