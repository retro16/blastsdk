//blsgen_gcc
void section_create_gcc(group *source, const mdconfnode *mdconf); // Create a new section and parse it from conf
void source_get_symbols_gcc(group *s); // Get the list of internal and external symbols
void source_premap_gcc(group *s); // Compute final values (chip, bus, ...) for the source and its sections
void source_get_symbol_values_gcc(group *s); // Get internal symbol values
void source_compile_gcc(group *s); // Final compilation step. Produces the binary.

//blsgen_png
void section_create_png(group *source, const mdconfnode *mdconf);

