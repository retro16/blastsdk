#include "bls.h"

#include "png.h"

int compute_size_png(int srcid);
void compile_png(int srcid);
void link_png(int srcid);

void open_png(int srcid, FILE **outfp, png_structp *outptr, png_infop *outinfo);
void close_png(FILE *f, png_structp png_ptr, png_infop info_ptr);
int read_png_palette(int srcid, char pal[32]);


int compute_size_png(int srcid)
{
  src_t *s = &src[srcid];
  sprintf(s->binname, TMPDIR"/%s.bin", s->sym.name);

  FILE *f;
  png_structp png_ptr;
  png_infop info_ptr;

  open_png(srcid, &f, &png_ptr, &info_ptr);
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
    printf("File %s size is not a multiple of 8.\n", s->name);
    exit(1);
  }

  close_png(f, png_ptr, info_ptr);

  char n[NAMELEN];
  snprintf(n, NAMELEN, "%s_WIDTH", s->sym.name);
  chipaddr_t wa = {chip_none, width};
  sym_set_int_chip(srcid, n, wa);
  snprintf(n, NAMELEN, "%s_HEIGHT", s->sym.name);
  chipaddr_t ha = {chip_none, width};
  sym_set_int_chip(srcid, n, ha);

  s->binsize = width * height / 2;

  return 1;
}

void compile_png(int srcid)
{
  (void)srcid;
}

void link_png(int srcid)
{
  src_t *s = &src[srcid];

  FILE *f;
  png_structp png_ptr;
  png_infop info_ptr;

  open_png(srcid, &f, &png_ptr, &info_ptr);
  if(png_get_color_type(png_ptr, info_ptr) != PNG_COLOR_TYPE_PALETTE)
  {
    printf("Invalid PNG format : %d.\n", png_get_color_type(png_ptr, info_ptr));
    exit(1);
  }

  png_uint_32 width, height;
  int bit_depth, color_type;

  // Read image size
  png_get_IHDR (png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, NULL, NULL, NULL);
  if(width % 8 || height % 8)
  {
    printf("File %s size is not a multiple of 8.\n", s->name);
    exit(1);
  }
  if(bit_depth < 8)
  {
    png_set_packing(png_ptr);
  }
  if(bit_depth > 8)
  {
    printf("Unsupported PNG %s : bit depth = %d.\n", s->name, bit_depth);
    exit(1);
  }
  
  png_colorp pngpal;
  int cnum;
  png_get_PLTE(png_ptr, info_ptr, &pngpal, &cnum);

  if(cnum > 16)
  {
    printf("Unsupported PNG %s : too many colors.\n", s->name);
    exit(1);
  }

  // Uncompress PNG and generate tiled image
  char *img = malloc(s->binsize);
  memset(img, '\0', s->binsize);
  unsigned int x, y;
  
  // Allocate enough for one line at 8bpp
  png_bytep row_pointer = (png_bytep)malloc(width);
  for(y = 0; y < height; ++y)
  {
    png_read_row(png_ptr, row_pointer, NULL);
    for(x = 0; x < width; ++x)
    {
      // Very crude and inefficient algorithm. I have a good CPU and no time to refine it :)
      char v = row_pointer[x];
      if(!(x & 1))
      {
        // Pixels on even columns are stored in MSB
        v <<= 4;
      }
      img[tilemap_addr8(x + y * width, width)] |= v;
    }
  }
  free(row_pointer);

  close_png(f, png_ptr, info_ptr);
  f = fopen(s->binname, "wb");
  if(!f)
  {
    printf("Error: cannot open output for %s.\n", s->name);
    exit(1);
  }
  for(y = 0; y < height; ++y)
  {
    fwrite(img + y * width / 2, width / 2, 1, f);
  }
  fclose(f);

  free(img);

  s->binsize = width * height / 2;
}


int compute_size_pngspr(int srcid)
{
//  src_t *s = &src[srcid];
  // TODO
  (void)srcid;
  return 1;
}

void compile_pngspr(int srcid)
{
//  src_t *s = &src[srcid];
  // TODO
  (void)srcid;
}

void link_pngspr(int srcid)
{
//  src_t *s = &src[srcid];
  // TODO
  (void)srcid;
}


int compute_size_pngpal(int srcid)
{
  src_t *s = &src[srcid];
  char pal[32];
  sprintf(s->binname, TMPDIR"/%s.pal", s->sym.name);

  int cc = read_png_palette(srcid, pal);
  s->binsize = cc * 2;

  return 1;
}

void compile_pngpal(int srcid)
{
  (void)srcid;
}

void link_pngpal(int srcid)
{
  FILE *f;
  src_t *s = &src[srcid];
  char pal[32];
  int cc = read_png_palette(srcid, pal);
  f = fopen(s->binname, "wb");
  if(!f)
  {
    printf("Error: Cannot open palette file for source %s.\n", s->name);
    exit(1);
  }
  fwrite(pal, 1, cc * 2, f);
  fclose(f);
}

void open_png(int srcid, FILE **f, png_structp *png_ptr, png_infop *info_ptr)
{
  src_t *s = &src[srcid];

  png_byte magic[8];
  *f = fopen(s->filename, "rb");
  if(!*f)
  {
    printf("Error: Cannot open PNG source %s.\n", s->filename);
    exit(1);
  }
  fread(magic, 1, sizeof(magic), *f);
  if(!png_check_sig(magic, sizeof(magic)))
  {
    printf("Error: %s is not a valid PNG file.\n", s->name);
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

// Reads a PNG and extracts its palette.
// Return value is the color count.
// Limited to 16 colors. Transparency is ignored.
// pal contains the palette in genesis format.
int read_png_palette(int srcid, char pal[32])
{
  FILE *f;
  png_structp png_ptr;
  png_infop info_ptr;

  open_png(srcid, &f, &png_ptr, &info_ptr);
  if(png_get_color_type(png_ptr, info_ptr) != PNG_COLOR_TYPE_PALETTE)
  {
    printf("Invalid PNG : no palette.\n");
    exit(1);
  }

  png_colorp pngpal;
  int cnum;

  png_get_PLTE(png_ptr, info_ptr, &pngpal, &cnum);

  int i;
  for(i = 0; i < cnum; ++i)
  {
    pal[i * 2] = (pngpal[i].blue >> 5) << 1;
    pal[i * 2 + 1] = (pngpal[i].green >> 5) << 5
                   | (pngpal[i].red >> 5) << 1;
  }

  close_png(f, png_ptr, info_ptr);

  return cnum;
}

