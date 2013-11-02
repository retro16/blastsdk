#include "bls.h"

extern int scd; // 0 = genesis, 1 = 1M, 2 = 2M, 3 = wram program

bus_t find_bus(chip_t chip);
chipaddr_t bus2chip(busaddr_t busaddr);
busaddr_t chip2bus(chipaddr_t chipaddr, bus_t bus);
busaddr_t translate(busaddr_t busaddr, bus_t target);
void chip_align(chipaddr_t *chip);
const char *chip_name(chip_t chip);


bus_t find_bus(chip_t chip)
{
  switch(chip)
  {
    case chip_none:
    case chip_max:
    case chip_pram:
    case chip_wram:
    case chip_zram:
      return bus_none;

    case chip_cart:
    case chip_bram:
    case chip_vram:
    case chip_ram:
      return bus_main;

    case chip_pcm:
      return bus_sub;
  }
  return bus_none;
}

chipaddr_t bus2chip(busaddr_t ba)
{
  chipaddr_t ca;
  ca.chip = chip_none;
  ca.addr = -1;

  switch(ba.bus)
  {
    case bus_none:
      ca.addr = ba.addr;
      return ca;
    case bus_main:
      if(ba.addr >= 0 && ((!scd && ba.addr < 0x400000) || ba.addr < 0x020000))
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
			if(scd >= 2)
			{
				// Word RAM in 2M mode
				if(ba.addr >= 0x200000 && ba.addr < 0x240000)
				{
					if(scd == 3)
					{
						ca.chip = chip_cart;
					}
					else
					{
						ca.chip = chip_wram;
					}
					ca.addr = ba.addr - 0x200000;
					return ca;
				}
			}
			if(scd)
			{
				if(ba.addr >= 0x020000 && ba.addr < 0x040000 && ba.bank >= 0 && ba.bank <= 3)
				{
					ca.chip = chip_pram;
					ca.addr = ba.bank * 0x020000 + ba.addr - 0x020000;
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

busaddr_t chip2bus(chipaddr_t ca, bus_t bus)
{
  busaddr_t ba;
  ba.bus = bus;
  ba.bank = -1;
  switch(ca.chip)
  {
    case chip_none:
      ba.bus = bus_none;
      ba.addr = ca.addr;
      return ba;
    case chip_cart:
      if(ca.addr < 0 || ca.addr >= MAXCARTSIZE) break;
			if(scd == 3)
			{
				if(ca.addr >= 0x040000)
				{
					break;
				}
				ca.addr += 0x200000;
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
					switch(scd)
					{
						case 0:
							break;
						case 1:
							break;
						case 2:
						case 3:
							ba.addr = 0x200000 + ca.addr;
							return ba;
					}
				break;
				case bus_sub:
					switch(scd)
					{
						case 0:
							break;
						case 1:
							break;
						case 2:
							break;
						case 3:
							break;
					}
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

busaddr_t translate(busaddr_t busaddr, bus_t target)
{
  return chip2bus(bus2chip(busaddr), target);
}

static sv_t hw_chip_start(chip_t chip, bus_t bus, int bank)
{
  switch(chip)
  {
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
			return 0;
    case chip_wram:
			return 0x200000;
    case chip_pcm:
      return 0;
  }
  return -1;
}

sv_t chip_start(chip_t chip, bus_t bus, int bank)
{
  sv_t start = hw_chip_start(chip, bus, bank);
  if(chip == chip_cart && bank < 1)
  {
    start += ROMHEADERSIZE;
  }

  return start;
}

static sv_t hw_chip_size(chip_t chip, bus_t bus, int bank)
{
  switch(chip)
  {
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
    case chip_ram:
      {
        int scdram = scd || pgmchip == chip_wram || pgmchip == chip_pram;
        if(bus == bus_z80)
        {
          if(scdram && bank == 511) return 0x7D00;
          return 0x8000;
        }
        return scdram ? 0xFD00 : 0x10000; // Avoid allocation over the interrupt vector table for SCD
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

sv_t chip_size(chip_t chip, bus_t bus, int bank)
{
  sv_t size = hw_chip_size(chip, bus, bank);

  if(chip == chip_cart && bank < 1)
  {
    size -= ROMHEADERSIZE;
  }

  return size;
}

sv_t align_value(sv_t value, sv_t step)
{
  if(value % step)
  {
    value += step - (value % step);
  }
  return value;
}


void chip_align(chipaddr_t *chip)
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
      align = 1;
      break;
    case chip_vram:
      align = 32;
      break;
    case chip_ram:
      align = 2;
      break;
    case chip_pram:
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

const char *chip_name(chip_t chip)
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
    case chip_ram:
     return "ram";
    case chip_pram:
     return "pram";
    case chip_wram:
     return "wram";
    case chip_pcm:
     return "pcm";
  }
  return "unknown";
}

sv_t tilemap_addr4(sv_t addr, sv_t width)
{
  // Absolute coordinates
  sv_t x = addr % (width / 2);
  sv_t y = addr / (width / 2);

  sv_t tw = width / 8;

  // Tile index
  sv_t txi = x / 8;
  sv_t tyi = y / 8;

  // Tile offset
  sv_t txo = x % 8;
  sv_t tyo = y % 8;

  //     tiles on top      tiles on left   line offset in tile
  return (tyi * 32 * tw) + txi * 32      + tyo * 4             + txo / 2;
}

sv_t tilemap_addr8(sv_t addr, sv_t width)
{
  // Absolute coordinates
  sv_t x = addr % width;
  sv_t y = addr / width;

  // Tiles per line
  sv_t tw = width / 8;

  // Tile index
  sv_t txi = x / 8;
  sv_t tyi = y / 8;

  // Tile offset
  sv_t txo = x % 8;
  sv_t tyo = y % 8;

  //     tiles on top      tiles on left   line offset in tile
  return (tyi * 32 * tw) + txi * 32      + tyo * 4             + txo / 2;
}

