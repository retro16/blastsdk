#ifndef BLSADDRESS_H
#define BLSADDRESS_H

#include "bls.h"
#include "blsll.h"

// A bus is an address space viewed from a CPU
typedef enum bus {
  bus_none,
  bus_main,
  bus_sub,
  bus_z80,
  bus_max
} bus;
BLSENUM(bus, 8)

// A chip is a memory space
typedef enum chip {
  chip_none,
  chip_mstk, // Pseudo-chip : push data onto main stack.
  chip_sstk, // Pseudo-chip : push data onto sub stack.
  chip_zstk, // Pseudo-chip : push data onto z80 stack.
  chip_cart,
  chip_bram, // Optional genesis in-cartridge battery RAM
  chip_zram,
  chip_vram,
  chip_cram, // Color VRAM
  chip_ram,
  chip_pram,
  chip_wram,
  chip_pcm,
  chip_max
} chip;
BLSENUM(chip, 8)

typedef struct bankconfig {
  bus bus;
  int bank[chip_max]; // Gives bank based on chip
} bankconfig;

typedef struct busaddr {
  bus bus;
  sv addr;
  int bank; // -1 = unknown
} busaddr;

typedef struct chipaddr {
  chip chip;
  sv addr;
} chipaddr;

bus find_bus(chip chip);
chipaddr bus2chip(busaddr busaddr);
busaddr chip2bus(chipaddr chipaddr, bus bus);
busaddr translate(busaddr busaddr, bus target);
chipaddr bank2chip(const bankconfig *bankconfig, sv addr);
bus guessbus(chip chip);
sv chip2bank(chipaddr chipaddr, bankconfig *bankconfig);
void bankreset(bankconfig *bankconfig);
int chipaddr_reachable(chipaddr chipaddr, const bankconfig *bankconfig);
chipaddr phys2chip(sv physaddr);
busaddr phys2bus(sv physaddr, bus bus);
sv chip2phys(chipaddr ca);
chipaddr bankmove(chipaddr addr, bus bus, int newbank);
void chip_align(chipaddr *chip);
const char *chip_name(chip chip);
sv chip_start(chip chip);
sv chip_size(chip chip);
sv tilemap_addr4(sv addr, sv width);
sv tilemap_addr8(sv addr, sv width);
sv align_value(sv value, sv step);

#endif
