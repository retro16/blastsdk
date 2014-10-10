#ifndef BLS_H
#define BLS_H

#include <stdint.h>
#include "blsll.h"

#ifndef BDB_RAM
#define BDB_RAM 0xFFFDC0 // RAM address of bdb
#endif

#ifndef REGADDR
#define REGADDR BDB_RAM + 0x1FA // Offset of register table in debugger RAM
#endif

#ifndef BDB_SUB_RAM
#define BDB_SUB_RAM 0xC0
#endif

#ifndef SUBREGADDR
#define SUBREGADDR BDB_SUB_RAM
#endif

#ifndef SUB_SCRATCH
#define SUB_SCRATCH 0x1D0 // RAM that can be used as sdcratch pad. Used to make the sub CPU run code.
#endif

// Register identifiers
#define REG_D(n) (n)
#define REG_A(n) (REG_D(8) + (n))
#define REG_SP REG_A(7)
#define REG_PC REG_A(8)
#define REG_SR REG_A(9)

// Machine defines
#define VDPDATA 0xC00000
#define VDPCTRL 0xC00004
#define VDPR_AUTOINC 15

// Format defines
#define ROMHEADERSIZE 0x200
#define MAXCARTSIZE 0x400000
#define CDHEADERSIZE 0x200 // Size of CD
#define SECCODESIZE 0x584 // Size of security code
#define SPHEADERSIZE 0x28 // Size of SP header
#define IPOFFSET (SECCODESIZE + 6) // chip address of IP binary in RAM
#define SPOFFSET (0x6000 + SPHEADERSIZE) // chip address of SP binary in PRAM
#define CDBLOCKSIZE 2048 // ISO block size
#define MINCDBLOCKS 300 // Minimum number of blocks in a CD-ROM
#define MAXCDBLOCKS 270000 // Maximum number of blocks in a CD-ROM

#ifndef MAINSTACKSIZE
#define MAINSTACKSIZE 0x100 // Default stack size
#endif
#ifndef MAINSTACK
#define MAINSTACK 0xFD00 // Default stack pointer
#endif

#ifndef BLSPREFIX
#error BLSPREFIX not defined
#endif

#ifndef BUILDDIR
#define BUILDDIR "build_blsgen"
#endif

// Types
typedef uint32_t u32;
typedef uint8_t u8;
typedef int64_t sv;

// Globals

typedef enum target {
  target_unknown,
  target_gen,
  target_scd1, // SCD in 1M mode
  target_scd2, // SCD in 2M mode
  target_ram,
  target_max
} target;
BLSENUM(target, 8)

extern target maintarget; // Defined in blsaddress.c


// Static inlines

static inline u32 getint(const u8 *data, int size)
{
  u32 d = 0;
  int i;

  for(i = 0; i < size; ++i) {
    d |= data[i] << ((size - 1 - i) * 8);
  }

  return d;
}

static inline void signext(int *v, int bits)
{
  if(*v & (1<<(bits-1))) {
    *v |= (-1) << bits;
  } else {
    *v &= (1L << bits) - 1;
  }
}

static inline void setint(u32 value, u8 *target, int size)
{
  int i;

  for(i = 0; i < size; ++i) {
    target[i] = value >> ((size - 1 - i) * 8);
  }
}

static inline sv neint(sv v)
{
  if(v < 0) {
    return v;
  }

  return (-v) & 0xFFFFFFFF;
}

static inline sv not_int(sv v)
{
  if(v < 0) {
    return v;
  }

  return (~v) & 0xFFFFFFFF;
}

#endif//BLS_H

// vim: ts=2 sw=2 sts=2 et
