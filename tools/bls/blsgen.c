#include "blsll.h"
#include "blsgen.h"
#include "blsconf.h"

group * group_new() {
  return (group *)malloc(sizeof(group));
}

void group_free(group *p) {
  if(p->name) free(p->name);
  free(p);
}

const char bus_names[][8] = {"none", "main", "sub", "z80"};
const char chip_names[][8] = {"none", "stack", "cart", "bram", "zram", "vram", "ram", "pram", "wram", "pcm"};
const char format_names[][8] = {"auto", "empty", "zero", "raw", "asmx", "sdcc", "gcc", "as", "png"};

section * section_new() {
  return (section *)malloc(sizeof(section));
}

void section_free(section *p) {
  if(p->name) free(p->name);
  if(p->datafile) free(p->datafile);
  free(p);
}

symbol * symbol_new() {
  return (symbol *)malloc(sizeof(symbol));
}

void symbol_free(symbol *p) {
  if(p->name) free(p->name);
  free(p);
}

const char target_name[][8] = {"gen", "scd", "vcart"};

void output_free(output *p) {
  if(p->name) free(p->name);
  if(p->region) free(p->region);
  if(p->file) free(p->file);
  free(p);
}

int main() {
  blsconf_load("blsgen.md");

}
