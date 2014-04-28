#include "blsgen.h"
#include "blsgen_ext.h"
#include "blsconf.h"

#include <unistd.h>
#include "png.h"

void open_png(const char *filename, FILE **f, png_structp *png_ptr, png_infop *info_ptr)
{
  png_byte magic[8];
  *f = fopen(filename, "rb");
  if(!*f)
  {
    printf("Error: Cannot open PNG source %s.\n", filename);
    exit(1);
  }
  fread(magic, 1, sizeof(magic), *f);
  if(!png_check_sig(magic, sizeof(magic)))
  {
    printf("Error: %s is not a valid PNG file.\n", filename);
    exit(1);
  }

  *png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if(!*png_ptr)
  {
    printf("Error: cannot init libpng.\n");
    exit(2);
  }
  *info_ptr = png_create_info_struct(*png_ptr);
  if(!*png_ptr)
  {
    printf("Error: cannot init libpng info.\n");
    exit(2);
  }

  png_init_io(*png_ptr, *f);
  png_set_sig_bytes(*png_ptr, sizeof(magic));
  png_read_info(*png_ptr, *info_ptr);
}

void close_png(FILE *f, png_structp png_ptr, png_infop info_ptr)
{
//  png_read_end(png_ptr, NULL);
  png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
  fclose(f);
}

/* blsgen functions to generate binaries from gcc sources */

void section_create_png(group *source, const mdconfnode *mdconf)
{
  (void)mdconf;
  section *s;

  // Generate the .img section
  source->provides = blsll_insert_section(source->provides, (s = section_parse_ext(NULL, source->name, ".img")));
  s->source = source;

  // Generate the .pal section
  source->provides = blsll_insert_section(source->provides, (s = section_parse_ext(NULL, source->name, ".pal")));
  s->source = source;

  // Generate the .map section
  source->provides = blsll_insert_section(source->provides, (s = section_parse_ext(NULL, source->name, ".map")));
  s->source = source;
  s->align = 0x2000;
  
  if(source->bus == bus_none && mainout.target != target_scd)
  {
    // For genesis, default to main bus
    source->bus = bus_main;
  }
}

void source_get_size_png(group *s)
{
  section *img = section_find_ext(s->name, ".img");
  section *pal = section_find_ext(s->name, ".pal");
  section *map = section_find_ext(s->name, ".map");

  FILE *f;
  png_structp png_ptr;
  png_infop info_ptr;

  open_png(s->name, &f, &png_ptr, &info_ptr);
  if(png_get_color_type(png_ptr, info_ptr) != PNG_COLOR_TYPE_PALETTE)
  {
    printf("Invalid PNG format : %d.\n", png_get_color_type(png_ptr, info_ptr));
    exit(1);
  }

  // Read image size
  png_uint_32 width = png_get_image_width(png_ptr, info_ptr);
  png_uint_32 height = png_get_image_height(png_ptr, info_ptr);
  if(width % 8 || height % 8)
  {
    printf("File %s size is not a multiple of 8 pixels.\n", s->name);
    exit(1);
  }

  close_png(f, png_ptr, info_ptr);

  if(s->optimize > 0) {
    printf("Error : optimized PNG is unsupported.\n");
    exit(1);
  }

  img->size = width * height / 2;
  pal->size = 16 * 2; // 16 colors palette
  map->size = compute_map_size(width, height);
}

void source_get_symbols_png(group *s)
{
  source_get_size_png(s);
}


void source_premap_png(group *s)
{
  section *img = section_find_ext(s->name, ".img");
  section *pal = section_find_ext(s->name, ".pal");
  section *map = section_find_ext(s->name, ".map");

  if(img->symbol->value.chip == chip_none)
  {
    switch(s->bus) {
      case bus_none:
      case bus_max:
        break;
      case bus_z80:
      case bus_main:
        img->symbol->value.chip = chip_vram;
        break;
      case bus_sub:
        img->symbol->value.chip = chip_wram;
        break;
    }
  }

  if(pal->symbol->value.chip == chip_none)
  {
    switch(s->bus) {
      case bus_none:
      case bus_max:
        break;
      case bus_z80:
      case bus_main:
        pal->symbol->value.chip = chip_cram;
        break;
      case bus_sub:
        pal->symbol->value.chip = chip_wram;
        break;
    }
  }

  if(map->symbol->value.chip == chip_none)
  {
    switch(s->bus) {
      case bus_none:
      case bus_max:
        break;
      case bus_z80:
      case bus_main:
        map->symbol->value.chip = chip_vram;
        break;
      case bus_sub:
        map->symbol->value.chip = chip_wram;
        break;
    }
  }
}
