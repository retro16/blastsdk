/* Blast! SDK main include
 */
#ifndef BLS_H
#define BLS_H

typedef unsigned char u8
typedef unsigned short u16
typedef unsigned long u32

// Bus constants
#define BUS_NONE 0
#define BUS_MAIN 1
#define BUS_SUB 2
#define BUS_Z80 3

// Chip constants
#define CHIP_NONE 0
#define CHIP_MSTACK 1
#define CHIP_SSTACK 2
#define CHIP_ZSTACK 3
#define CHIP_CART 4
#define CHIP_BRAM 5
#define CHIP_ZRAM 6
#define CHIP_VRAM 7
#define CHIP_RAM 8
#define CHIP_PRAM 9
#define CHIP_WRAM 10
#define CHIP_PCM 11

// Target constants
#define TARGET_GEN 0
#define TARGET_SCD 1
#define TARGET_RAM 2

#define TRAP(t) asm volatile("\tTRAP #"#t)

#if BUS == BUS_MAIN
#define ENTER_MONITOR() TRAP(7)

#if TARGET == TARGET_GEN
#define VDPSECURITY() \
asm volatile("\tmove.b %d0, -(%a7)\n" \
"\tmove.b 0xA10001, %d0\n" \
"\tandi.b #0x0F, %d0\n" \
"\tbeq.b 1f\n" \
"\tmove.l 0x100, 0xA14000\n" \
"1:\tmove.b (%a7)+, %d0\n" )
#else
#define VDPSECURITY() do {} while(0)
#endif

#endif // BSS_MAIN

#endif
