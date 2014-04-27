#include "blsgen.h"

bus find_bus(chip chip);
chipaddr bus2chip(busaddr busaddr);
busaddr chip2bus(chipaddr chipaddr, bus bus);
busaddr translate(busaddr busaddr, bus target);
chipaddr bankmove(chipaddr addr, bus bus, int newbank);
void chip_align(chipaddr *chip);
const char *chip_name(chip chip);
sv chip_start(chip chip);
sv chip_size(chip chip);
