#include "blsaddress.h"


bus find_bus(chip chip)
{
  switch(chip)
  {
    case chip_none:
    case chip_max:
    case chip_wram:
    case chip_sstack:
    case chip_zstack:
      return bus_none;

    case chip_zram:
      return bus_z80;

    case chip_cart:
    case chip_bram:
    case chip_vram:
    case chip_cram:
    case chip_ram:
    case chip_mstack:
      return bus_main;

    case chip_pram:
    case chip_pcm:
      return bus_sub;
  }
  return bus_none;
}

chipaddr bus2chip(busaddr ba)
{
  chipaddr ca;
  ca.chip = chip_none;
  ca.addr = -1;

  switch(ba.bus)
  {
    case bus_none:
      ca.addr = ba.addr;
      return ca;

    case bus_main:
      if(mainout.target == target_gen && ba.addr >= 0 && ba.addr < 0x400000)
      {
        ca.chip = chip_cart;
        ca.addr = ba.addr;
        return ca;
      }
      if(ba.addr >= 0xFE0000 && ba.addr < 0x1000000)
      {
        ca.chip = chip_ram;
        ca.addr = ba.addr & 0xFFFF;
        return ca;
      }
      if(ba.addr >= 0xA00000 && ba.addr < 0xA02000)
      {
        ca.chip = chip_zram;
        ca.addr = ba.addr - 0xA00000;
        return ca;
      }
      if(ba.addr >= 0x200000 && ba.addr < 0x240000)
      {
          ca.chip = chip_wram;
          ca.addr = ba.addr - 0x200000;
          return ca;
      }
      if(mainout.target == target_scd)
      {
        if(ba.addr >= 0x020000 && ba.addr < 0x040000 && ba.bank >= 0 && ba.bank <= 3)
        {
          ca.chip = chip_pram;
          ca.addr = ba.bank * 0x020000 + ba.addr - 0x020000;
          return ca;
        }
      }
      else if(mainout.target == target_vcart)
      {
        if(ba.addr >= 0x020000 && ba.addr < 0x040000)
        {
          ca.chip = chip_pram;
          ca.addr = ba.addr - 0x020000;
          return ca;
        }
      }
      break;

    case bus_z80:
      if(ba.addr >= 0 && ba.addr < 0x2000)
      {
        ca.chip = chip_zram;
        ca.addr = ba.addr;
        return ca;
      }
      if(ba.addr >= 0x8000 && ba.addr < 0x10000 && ba.bank >= 0 && ba.bank < 512)
      {
        // Remap as main bus and convert to chip
        ba.bus = bus_main;
        ba.addr = ba.addr | (ba.bank << 15);
        ba.bank = 0;
        return bus2chip(ba);
      }
      if(ba.addr >= 0x8000 && ba.addr < 0x10000 && ba.bank == -1)
      {
        // Unknown bank - use default
        ba.bus = bus_main;
        ba.addr = ba.addr;
        ba.bank = 0;
        return bus2chip(ba);
      }
      break;

    case bus_sub:
      if(ba.addr >= 0 && ba.addr < 0x80000)
      {
        ca.chip = chip_pram;
        ca.addr = ba.addr;
        return ca;
      }
      // TODO:other chips
      break;

    case bus_max:
      break;
  }

  return ca;
}

busaddr chip2bus(chipaddr ca, bus bus)
{
  busaddr ba;
  ba.bus = bus;
  ba.bank = -1;
  switch(ca.chip)
  {
    case chip_mstack:
    case chip_sstack:
    case chip_zstack:
      break;

    case chip_none:
      ba.bus = bus_none;
      ba.addr = ca.addr;
      return ba;
    case chip_cart:
      if(ca.addr < 0 || ca.addr >= MAXCARTSIZE) break;
      if(mainout.target == target_vcart)
      {
        if(ca.addr >= 0x020000)
        {
          break;
        }
        ca.addr += 0x020000;
        ba.bank = 0;
      } else if(mainout.target == target_ram)
      {
        if(ca.addr >= 0x00FD00)
        {
          break;
        }
        ca.addr += 0xFF0200;
      }
      switch(bus)
      {
        case bus_main:
          ba.addr = ca.addr;
          return ba;
        case bus_z80:
          ba.addr = ca.addr & 0x7FFF;
          ba.bank = ca.addr >> 15;
          return ba;
        default:
          break;
      }
      break;
    case chip_ram:
      if(ca.addr < 0 || ca.addr > 0xFFFF) break;
      switch(bus)
      {
        case bus_main:
          ba.addr = ca.addr + 0xFF0000;
          return ba;
        case bus_z80:
          ba.addr = ca.addr & 0x7FFF;
          ba.bank = ca.addr >> 15;
          return ba;
        default:
          break;
      }
      break;
    case chip_zram:
      if(ca.addr < 0 || ca.addr > 0x1FFF) break;
      switch(bus)
      {
        case bus_main:
          ba.addr = ca.addr + 0xA00000;
          return ba;
        case bus_z80:
          ba.addr = ca.addr;
          return ba;
        default:
          break;
      }
      break;
    case chip_vram:
    case chip_cram:
      break;
    // TODO : CD chips
    case chip_bram:
      break;
    case chip_pram:
      if(ca.addr < 0 || ca.addr >= 0x80000) break;
      switch(bus)
      {
        case bus_main:
          ba.addr = (ca.addr & 0x020000) | 0x020000;
          ba.bank = ca.addr >> 17;
          return ba;
        case bus_sub:
          ba.addr = ca.addr;
          return ba;
        default:
          break;
      }
    case chip_wram:
      if(ca.addr < 0 || ca.addr >= 0x040000) break;
      switch(bus)
      {
        default:
          break;
        case bus_main:
          switch(mainout.target)
          {
            case target_max:
              break;
            case target_gen:
            case target_ram:
              break;
            case target_scd:
            case target_vcart:
              ba.addr = 0x200000 + ca.addr;
              return ba;
          }
        break;
        case bus_sub:
          // TODO
        break;
      }
    case chip_pcm:
      break;
    case chip_max:
      break;
  }

  // Invalid combination
  ba.bus = bus_none;
  ba.addr = -1;
  ba.bank = -1;
  return ba;
}

busaddr translate(busaddr busaddr, bus target)
{
  return chip2bus(bus2chip(busaddr), target);
}
/*
static sv hw_chip_start(chip chip, bus bus, int bank)
{
  switch(chip)
  {
    case chip_mstack:
    case chip_sstack:
    case chip_zstack:
      break;

    case chip_none:
    case chip_max:
      return -1;
    case chip_cart:
      if(bus == bus_z80 && bank > 0)
      {
        return 0x8000 * bank;
      }
      return 0;
    case chip_bram:
      return -1; // TODO
    case chip_pram:
      if(bus == bus_main && bank > 0)
      {
        return 0x20000 * bank;
      }
      return 0x6000;
    case chip_ram:
      if(bus == bus_z80 && bank >= 510)
      {
        return (bank - 510) * 0x8000;
      }
      return 0;
    case chip_zram:
    case chip_vram:
    case chip_cram:
      return 0;
    case chip_wram:
      return 0x200000;
    case chip_pcm:
      return 0;
  }
  return -1;
}
*/
sv chip_start(chip chip)
{
  if(chip == chip_cart && mainout.target != target_vcart && mainout.target != target_ram)
  {
    return ROMHEADERSIZE;
  }
  else if(chip == chip_vram)
  {
    // Skip null tile
    return 0x20;
  }
  else if(chip == chip_pram)
  {
    // Skip CD BIOS
    return 0x6000;
  }

  return 0;
}

sv chip_size(chip chip)
{
  switch(chip) {
    case chip_bram:
      // TODO
    case chip_none:
    case chip_max:
      return -1;
    case chip_cart:
      if(mainout.target == target_vcart) {
        return chip_size(chip_pram);
      } else if(mainout.target == target_ram) {
        return chip_size(chip_ram);
      }
      return MAXCARTSIZE - ROMHEADERSIZE; // Avoid allocating over ROM header
    case chip_zstack:
    case chip_zram:
      return 0x2000;
    case chip_vram:
      return 0x10000;
    case chip_cram:
      return 0x80;
    case chip_mstack:
    case chip_ram:
      if(mainout.target == target_ram) {
        return 0xFD00;
      }
      if(mainout.target != target_gen) {
        return 0xFD00; // Avoid allocating over exception vectors
      }
      return 0xFFB6; // Avoid allocating over monitor CPU state
    case chip_sstack:
    case chip_pram:
      if(mainout.target == target_vcart) {
        return 0x20000;
      }
      return 0x80000 - 0x6000;
    case chip_wram:
      return 0x40000;
    case chip_pcm:
      return 0x10000;
  }
  return -1;
}
/*
static sv hw_chip_size(chip chip, bus bus, int bank)
{
  switch(chip)
  {
    case chip_mstack:
    case chip_sstack:
    case chip_zstack:
    case chip_none:
    case chip_max:
      return -1;
    case chip_cart:
      if(bus == bus_z80)
        return 0x8000;
      return MAXCARTSIZE;
    case chip_bram:
      return -1; // TODO
    case chip_zram:
      return 0x2000;
    case chip_vram:
      return 0x10000;
    case chip_cram:
      return 0x80;
    case chip_ram:
      {
        // Avoid allocation over the interrupt vector table for SCD
        if(bus == bus_z80)
        {
          if(mainout.target != target_gen && bank == 511) return 0x7D00;
          return 0x8000;
        }
        return mainout.target != target_gen ? 0xFD00 : 0x10000;
      }
    case chip_pram:
      if(bus == bus_main)
      {
        if(bank == 0)
        {
          return 0x20000 - 0x6000;
        }
        return 0x20000;
      }
      return 0x80000 - 0x6000;
    case chip_wram:
      return 0x40000;
    case chip_pcm:
      return 0x10000;
  }
  return -1;
}
*/
chipaddr bankmove(chipaddr addr, bus bus, int newbank)
{
  busaddr ba = chip2bus(addr, bus);
  if(ba.bank < 0)
  {
    return addr;
  }
  ba.bank = newbank;
  return bus2chip(ba);
}

sv align_value(sv value, sv step)
{
  if(value % step)
  {
    value += step - (value % step);
  }
  return value;
}


void chip_align(chipaddr *chip)
{
  int align;

  switch(chip->chip)
  {
    case chip_none:
    case chip_max:
      align = 1;
      break;
    case chip_cart:
      align = 2;
      break;
    case chip_bram:
      align = 2;
      break;
    case chip_zram:
    case chip_zstack:
      align = 1;
      break;
    case chip_vram:
      align = 32;
      break;
    case chip_cram:
      align = 32;
      break;
    case chip_ram:
    case chip_mstack:
      align = 2;
      break;
    case chip_pram:
    case chip_sstack:
      align = 2;
      break;
    case chip_wram:
      align = 2;
      break;
    case chip_pcm:
      align = 1;
      break;
  }

  chip->addr = align_value(chip->addr, align);
}

const char *chip_name(chip chip)
{
  switch(chip)
  {
    case chip_none:
     return "none";
    case chip_max:
     return "max";
    case chip_cart:
     return "cart";
    case chip_bram:
     return "bram";
    case chip_zram:
     return "zram";
    case chip_vram:
     return "vram";
    case chip_cram:
     return "cram";
    case chip_ram:
     return "ram";
    case chip_pram:
     return "pram";
    case chip_wram:
     return "wram";
    case chip_pcm:
     return "pcm";
    case chip_mstack:
     return "mstack";
    case chip_sstack:
     return "sstack";
    case chip_zstack:
     return "zstack";
  }
  return "unknown";
}

sv tilemap_addr4(sv addr, sv width)
{
  // Absolute coordinates
  sv x = addr % (width / 2);
  sv y = addr / (width / 2);

  sv tw = width / 8;

  // Tile index
  sv txi = x / 8;
  sv tyi = y / 8;

  // Tile offset
  sv txo = x % 8;
  sv tyo = y % 8;

  //     tiles on top      tiles on left   line offset in tile
  return (tyi * 32 * tw) + txi * 32      + tyo * 4             + txo / 2;
}

sv tilemap_addr8(sv addr, sv width)
{
  // Absolute coordinates
  sv x = addr % width;
  sv y = addr / width;

  // Tiles per line
  sv tw = width / 8;

  // Tile index
  sv txi = x / 8;
  sv tyi = y / 8;

  // Tile offset
  sv txo = x % 8;
  sv tyo = y % 8;

  //     tiles on top      tiles on left   line offset in tile
  return (tyi * 32 * tw) + txi * 32      + tyo * 4             + txo / 2;
}

busaddr phys2bus(sv physaddr, bus target)
{
  busaddr ba = {bus_none, -1, -1};
  if(mainout.target != target_scd)
  {
    ba.bus = target;
    chipaddr ca = {chip_cart, physaddr};
    ba = chip2bus(ca, target);
  }

  return ba;
}
