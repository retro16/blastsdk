//blsgen_gcc
void section_create_gcc(group *source, const mdconfnode *mdconf); // Create a new section and parse it from conf
void source_get_symbols_gcc(group *s); // Get the list of internal and external symbols
void source_premap_gcc(group *s); // Compute final values (chip, bus, ...) for the source and its sections
void source_get_symbol_values_gcc(group *s); // Get internal symbol values
void source_compile_gcc(group *s); // Final compilation step. Produces the binary.

//blsgen_asmx
void section_create_asmx(group *source, const mdconfnode *mdconf); // Create a new section and parse it from conf
void source_get_symbols_asmx(group *s); // Get the list of internal and external symbols
void source_premap_asmx(group *s); // Compute final values (chip, bus, ...) for the source and its sections
void source_get_symbol_values_asmx(group *s); // Get internal symbol values
void source_compile_asmx(group *s); // Final compilation step. Produces the binary.

//blsgen_png
void section_create_png(group *source, const mdconfnode *mdconf);
void source_get_symbols_png(group *s);
void source_premap_png(group *s);
void source_compile_png(group *s);

//blsgen_img : generic image manipulation
sv compute_map_size(unsigned int width, unsigned int height);
void gen_simple_map(char *out, sv tiles_ram_offset, sv null_tile_offset, unsigned int width, unsigned int height);

//blsgen_raw : raw file
void section_create_raw(group *source, const mdconfnode *mdconf);
void source_get_symbols_raw(group *s);
void source_premap_raw(group *s);
void source_compile_raw(group *s);

// vim: ts=2 sw=2 sts=2 et
