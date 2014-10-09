#include "blsgen.h"
#include "blsgen_ext.h"
#include "blsconf.h"
#include "blsaddress.h"

#include <unistd.h>
#include "png.h"

void open_png(const char *filename, FILE **f, png_structp *png_ptr, png_infop *info_ptr)
{
  png_byte magic[8];
  *f = fopen(filename, "rb");

  if(!*f) {
    printf("Error: Cannot open PNG source %s.\n", filename);
    exit(1);
  }

  fread(magic, 1, sizeof(magic), *f);

  if(!png_check_sig(magic, sizeof(magic))) {
    printf("Error: %s is not a valid PNG file.\n", filename);
    exit(1);
  }

  *png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

  if(!*png_ptr) {
    printf("Error: cannot init libpng.\n");
    exit(2);
  }

  *info_ptr = png_create_info_struct(*png_ptr);

  if(!*png_ptr) {
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
  source->provides = blsll_insert_section(source->provides, (s = section_parse_ext(mdconf, source->name, ".img")));
  s->source = source;

  // Generate the .pal section
  source->provides = blsll_insert_section(source->provides, (s = section_parse_ext(mdconf, source->name, ".pal")));
  s->source = source;

  // Generate the .map section
  source->provides = blsll_insert_section(source->provides, (s = section_parse_ext(mdconf, source->name, ".map")));
  s->source = source;
  s->align = 0x2000;

  if(source->banks.bus == bus_none && maintarget != target_scd1 && maintarget != target_scd2) {
    // For genesis, default to main bus
    source->banks.bus = bus_main;
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
  char srcname[4096];
  findfile(srcname, s->file);

  open_png(srcname, &f, &png_ptr, &info_ptr);

  if(png_get_color_type(png_ptr, info_ptr) != PNG_COLOR_TYPE_PALETTE) {
    printf("Invalid PNG format : %d.\n", png_get_color_type(png_ptr, info_ptr));
    exit(1);
  }

  // Read image size
  png_uint_32 width = png_get_image_width(png_ptr, info_ptr);
  png_uint_32 height = png_get_image_height(png_ptr, info_ptr);

  if(width % 8 || height % 8) {
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

  if(img->symbol->value.chip == chip_none) {
    switch(s->banks.bus) {
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

  if(pal->symbol->value.chip == chip_none) {
    switch(s->banks.bus) {
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

  if(map->symbol->value.chip == chip_none) {
    switch(s->banks.bus) {
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

void source_compile_png(group *s)
{
  section *img = section_find_ext(s->name, ".img");
  section *pal = section_find_ext(s->name, ".pal");
  section *map = section_find_ext(s->name, ".map");

  FILE *f;
  png_structp png_ptr;
  png_infop info_ptr;
  char srcname[4096];
  findfile(srcname, s->file);

  open_png(srcname, &f, &png_ptr, &info_ptr);

  if(png_get_color_type(png_ptr, info_ptr) != PNG_COLOR_TYPE_PALETTE) {
    printf("Invalid PNG format : %d.\n", png_get_color_type(png_ptr, info_ptr));
    exit(1);
  }

  if(png_get_color_type(png_ptr, info_ptr) != PNG_COLOR_TYPE_PALETTE) {
    printf("Invalid PNG format : %d.\n", png_get_color_type(png_ptr, info_ptr));
    exit(1);
  }

  png_uint_32 width, height;
  int bit_depth, color_type;

  // Read image size
  png_get_IHDR (png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, NULL, NULL, NULL);

  if(width % 8 || height % 8) {
    printf("File %s size is not a multiple of 8.\n", s->name);
    exit(1);
  }

  if(bit_depth < 8) {
    png_set_packing(png_ptr);
  }

  if(bit_depth > 8) {
    printf("Unsupported PNG %s : bit depth = %d.\n", s->name, bit_depth);
    exit(1);
  }

  png_colorp pngpal;
  int cnum;
  png_get_PLTE(png_ptr, info_ptr, &pngpal, &cnum);

  if(cnum > 16) {
    printf("Unsupported PNG %s : too many colors.\n", s->name);
    exit(1);
  }

  char paldata[16*2];
  memset(paldata, '\0', sizeof(paldata));

  // Generate palette file
  int i;

  for(i = 0; i < cnum; ++i) {
    paldata[i * 2] = (pngpal[i].blue >> 5) << 1;
    paldata[i * 2 + 1] = (pngpal[i].green >> 5) << 5
                         | (pngpal[i].red >> 5) << 1;
  }

  char outfilename[4096];
  snprintf(outfilename, 4096, BUILDDIR"/%s", pal->name);
  FILE *outfile = fopen(outfilename, "wb");

  if(!outfile) {
    printf("Error: cannot open output for %s.\n", pal->name);
    exit(1);
  }

  fwrite(paldata, sizeof(paldata), 1, outfile);
  fclose(outfile);

  // Uncompress PNG and generate tiled image
  char *imgdata = malloc(img->size);
  memset(imgdata, '\0', img->size);
  unsigned int x, y;

  // Allocate enough for one line at 8bpp
  png_bytep row_pointer = (png_bytep)malloc(width);

  for(y = 0; y < height; ++y) {
    png_read_row(png_ptr, row_pointer, NULL);

    for(x = 0; x < width; ++x) {
      // Very crude and inefficient algorithm. I have a good CPU and no time to refine it :)
      char v = row_pointer[x];

      if(!(x & 1)) {
        // Pixels on even columns are stored in MSB
        v <<= 4;
      }

      imgdata[tilemap_addr8(x + y * width, width)] |= v;
    }
  }

  free(row_pointer);

  // Copy pixels to output file
  snprintf(outfilename, 4096, BUILDDIR"/%s", img->name);
  outfile = fopen(outfilename, "wb");

  if(!outfile) {
    printf("Error: cannot open output for %s.\n", img->name);
    exit(1);
  }

  for(y = 0; y < height; ++y) {
    fwrite(imgdata + y * width / 2, width / 2, 1, outfile);
  }

  fclose(outfile);

  free(imgdata);


  // Generate image map
  char *mapdata = malloc(map->size);
  memset(mapdata, '\0', map->size);

  gen_simple_map(mapdata, img->symbol->value.addr, 0, width, height);

  snprintf(outfilename, 4096, BUILDDIR"/%s", map->name);
  outfile = fopen(outfilename, "wb");
  printf("Output map (%08X) to %s\n", (unsigned int)map->size, outfilename);

  if(!outfile) {
    printf("Error: cannot open output for %s.\n", map->name);
    exit(1);
  }

  printf("%lu\n", fwrite(mapdata, 1, map->size, outfile));
  fclose(outfile);

  free(mapdata);

  close_png(f, png_ptr, info_ptr);
}

// vim: ts=2 sw=2 sts=2 et
