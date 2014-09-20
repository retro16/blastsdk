#ifndef BLS_H
#define BLS_H

#include <stdint.h>

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
#define SUB_SCRATCH 0x1F0 // Writable part of the sub CPU program ram
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

// Types
typedef uint32_t u32;
typedef uint8_t u8;

#endif//BLS_H
