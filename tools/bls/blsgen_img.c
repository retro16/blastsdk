#include "blsgen.h"

static int compute_cells(unsigned int pixels) {
	if(pixels <= 256) return 32;
	if(pixels <= 512) return 64;
	if(pixels <= 1024) return 128;
	return 0;
}

sv compute_map_size(unsigned int width, unsigned int height) {
	int hcells = compute_cells(width);
	int vcells = compute_cells(height);

	return hcells * vcells * 2;
}

// Generates a simple map for a given image. The image will be positioned at the top-left hand corner of the map.
// tiles_ram_offset is the VRAM address of the first tile. All tiles must be contiguous, in reading order (left to right, top to bottom)
// null_tile_offset is used to fill cells that are outside the image.
// out must be at least 'compute_map_size' bytes long.
void gen_simple_map(char *out, sv tiles_ram_offset, sv null_tile_offset, unsigned int width, unsigned int height) {
	int hcells = compute_cells(width);
	int vcells = compute_cells(height);
	int img_hcells = (width + 7) / 8;
	int img_vcells = (height + 7) / 8;
	tiles_ram_offset /= 32; // Compute ram offset in cells.
	null_tile_offset /= 32;

	int x, y;
	for(y = 0; y < img_vcells; ++y) {
		for(x = 0; x < img_hcells; ++x) {
			int cellindex = (x + y * hcells) * 2;
			int cellid = x + y * img_hcells + tiles_ram_offset;
			out[cellindex] = cellid >> 8;
			out[cellindex + 1] = cellid & 0xFF;
		}
		for(; x < hcells; ++x) {
			int cellindex = (x + y * hcells) * 2;
			out[cellindex] = null_tile_offset >> 8;
			out[cellindex + 1] = null_tile_offset & 0xFF;
		}
	}
	for(; y < vcells; ++y) {
		for(x = 0; x < hcells; ++x) {
			int cellindex = (x + y * hcells) * 2;
			out[cellindex] = null_tile_offset >> 8;
			out[cellindex + 1] = null_tile_offset & 0xFF;
		}
	}
}
