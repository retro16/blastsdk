#ifndef BLSBUILD_H
#define BLSBUILD_H

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <ctype.h>

#define NAMELEN 128 // Length of symbol and file names

#define ROMHEADERSIZE 0x200
#define MAXCARTSIZE 0x400000

#define CDHEADERSIZE 0x200 // Size of CD
#define SECCODESIZE 0x584 // Size of security code
#define SPHEADERSIZE 0x28 // Size of SP header
#define CDBLOCKSIZE 2048 // ISO block size
#define CDBLOCKCNT 360000 // Number of blocks in a 80 minutes CD-ROM

#ifndef MAINSTACK
#define MAINSTACK 0xFFF800 // Default stack pointer
#endif

#define SRCCNT 1024 // Maximum number of sources
#define SYMCNT 1024 // Maximum number of symbols (external or internal) per source

#ifndef TMPDIR
#define TMPDIR "tmp"
#endif

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

#define QUOTE(X) #X
#define STRINGIFY(X) QUOTE(X)

// Machine defines
#define VDPDATA 0xC00000
#define VDPCTRL 0xC00004
#define VDPR_AUTOINC 15

////// Globals

typedef uint32_t u32;
typedef uint8_t u8;
typedef int64_t i64;

// Symbol value
typedef i64 sv_t;

static inline u32 getint(const u8 *data, int size)
{
  u32 d = 0;
  int i;
  for(i = 0; i < size; ++i)
  {
    d |= data[i] << ((size - 1 - i) * 8);
  }
  return d;
}

static inline void signext(int *v, int bits)
{
  if(*v & (1<<(bits-1)))
  {
    *v |= (-1) << bits;
  }
  else
  {
    *v &= (1L << bits) - 1;
  }
}

static inline void setint(u32 value, u8 *target, int size)
{
  int i;
  for(i = 0; i < size; ++i)
  {
    target[i] = value >> ((size - 1 - i) * 8);
  }
}

////// utils.c : Utility functions //////
extern size_t filesize(const char *file);
extern size_t getbasename(char *output, const char *name);
extern size_t getext(char *output, const char *name);
extern size_t getsymname(char *output, const char *path);
extern int nextpoweroftwo(int v);
extern int fileexists(const char *f);
extern time_t filedate(const char *f);
extern int hex2bin(u8 *code, u8 *c);
extern int readfile(const char *name, u8 **data);
extern int writefile(const char *name, const u8 *data, int size);
extern void skipblanks(const char **l);
extern void parse_blank(const char **cp);
extern sv_t parse_hex(const char **cp, int len);
extern sv_t parse_int(const char **cp, int len);
extern void parse_sym(char *s, const char **cp); // Parse symbol name from cp to s and move cp after it
extern void edit_text_file(const char *filename);
extern void hexdump(const u8 *data, int size, u32 offset);


////// address.c : Bus and addresses //////

// A bus is an address space viewed from a CPU
typedef enum bus {
  bus_none = -1,
  bus_main,
  bus_sub,
  bus_z80,
  bus_max
} bus_t;

// A chip is a memory space
typedef enum chip {
  chip_none = -1,
  chip_cart,
  chip_bram, // Genesis in-cartridge battery RAM
  chip_zram,
  chip_vram,
  chip_ram,
  chip_pram,
  chip_wram,
  chip_pcm,
  chip_max
} chip_t;

typedef struct busaddr {
  bus_t bus;
  sv_t addr;
  int bank;
} busaddr_t;

typedef struct chipaddr {
  chip_t chip;
  sv_t addr;
} chipaddr_t;


// Find most probable access bus based on chip
extern bus_t find_bus(chip_t chip);

// Convert from bus address to chip address
// This may fail if bank is unknown
extern chipaddr_t bus2chip(busaddr_t busaddr);

// Convert from chip address to bus address
extern busaddr_t chip2bus(chipaddr_t chipaddr, bus_t bus);

// Convert from one bus address to another
// Returns addr == -1 if impossible
extern busaddr_t translate(busaddr_t busaddr, bus_t target);

// Returns first available address for a given chip
extern sv_t chip_start(chip_t chip, bus_t bus, int bank);

// Returns the size of a chip in bytes
extern sv_t chip_size(chip_t chip, bus_t bus, int bank);

// Aligns a value to the next chunk of step
extern sv_t align_value(sv_t value, sv_t step);

// Aligns an address to the next position
extern void chip_align(chipaddr_t *chip);

// Returns the name of the chip
extern const char *chip_name(chip_t chip);

// Map from 8 bits linear framebuffer to tiled framebuffer
extern sv_t tilemap_addr8(sv_t addr, sv_t width);

// Map from 4 bits linear framebuffer to tiled framebuffer
extern sv_t tilemap_addr4(sv_t addr, sv_t width);

////// source.c : Program and data sources //////

#define GRPCNT ((int)sizeof(uint64_t) * 8)

typedef enum type {
  type_none = -1,
  type_empty, // Fixed allocation with no data
  type_bin, // Binary file
  type_asm, // asmx asm file
  type_c_m68k, // gcc m68k c source
  type_cxx_m68k, // gcc m68k c++ source
  type_elf, // m68k elf binary
  type_as, // gnu as (assembler) source
  type_c_z80, // sdcc z80 c source
  type_png, // PNG background graphics
  type_pngspr, // Square (32x32, 16x16, 8x8) sprite tileset
  type_pngpal, // Extract palette from PNG to genesis palette format
  type_max
} type_t;

typedef struct sym {
  char name[NAMELEN];
  chipaddr_t value; // Value as a translatable address, chip_none if a raw int value
} sym_t;

typedef struct src {
  type_t type;

  char name[NAMELEN]; // Source internal name
  char filename[NAMELEN]; // Source file name
  char binname[NAMELEN]; // Binary file name (object file to be written on CD)

  sym_t sym; // Symbol representing the source itself
  char extsym[SYMCNT][NAMELEN]; // External symbols
  sym_t intsym[SYMCNT]; // Internal symbols
  int align; // Alignment in bytes, -1 for default chip alignment
  int store; // True if data should be stored on physical medium
  bus_t bus; // Bus used to access resource at runtime
  int bank; // Bank used to access bus address
  int physaddr; // Physical address in bytes
  int physalign; // Alignment on physical medium, in bytes
  ssize_t binsize; // Binary file size
  char concat[NAMELEN]; // Next source in concatenation chain
  chipaddr_t concataddr;

  uint64_t groups; // Bitfield used for resource overlapping (bit set = used, bit clear = unused)
  int dirty; // True if resource changed since last run
  int mapped; // 1 if mapped automatically, -1 otherwise
} src_t;

extern src_t src[SRCCNT];
extern char groups[GRPCNT][NAMELEN]; // Group names
extern uint64_t groupmask; // Enabled groups
extern time_t inidate; // ini file date
extern time_t outdate; // output file date (0 if not found)

// Global CD boot sources. Treated specially.
extern char ipname[NAMELEN];
extern char spname[NAMELEN];
extern int ipsrc;
extern int spsrc;

extern size_t to_symname(char *output, const char *path);

extern int find_group(const char *name);
extern int new_group(const char *name);
extern uint64_t parse_groups(const char *list);
extern int src_overlap(int s); // True if any source overlap the source passed as parameter

// Symbol functions
extern int sym_get_value(chipaddr_t *out, const char *name);
busaddr_t sym_get_addr(const char *name, bus_t bus);
extern int sym_set_int(int source, const char *name, busaddr_t value);
extern int sym_set_int_chip(int source, const char *name, chipaddr_t value);
extern int sym_set_ext(int source, const char *name);
extern int sym_is_int(int source, const char *name);
extern int sym_is_ext(int source, const char *name);

// Source manipulation functions
extern void src_clear(int srcid); // Delete a source
extern void src_clear_sym(int srcid); // Delete all symbols of a source
extern int src_get(const char *name); // Get a source by file name
extern int src_get_sym(const char *name); // Get a source by symbol name
extern int src_add(const char *name); // Add a new source and return its index
extern void src_dirty_deps();
extern void src_map_addr(); // Compute addr for all unallocated sources
extern void src_map_phys(); // Compute physaddr for all unallocated sources



////// config.c : Global configuration //////

// Compilation tools
extern char cc[1024];
extern char cxx[1024];
extern char cflags[1024];
extern char as[1024];
extern char asflags[1024];
extern char ld[1024];
extern char ldflags[1024];
extern char objcopy[1024];
extern char nm[1024];
extern char asmx[1024];

extern char author[256];
extern char title[256];
extern char masterdate[16]; // MMDDYYYY format
extern char region[4]; // 'J', 'U' or 'E' or any combination. Only the first is used for scd.
extern char copyright[17];
extern char serial[17]; // Serial number
extern int scd; // 0 = no SCD, 1 = SCD mode 1M, 2 = SCD mode 2M  (mode may change during runtime)
extern chip_t pgmchip; // Genesis only. Chip that contains the cart image.

extern bus_t parse_bus(const char *s);
extern chip_t parse_chip(const char *s);
extern type_t parse_type(const char *s);
extern int parse_addr(const char *s);
extern int parse_physaddr(const char *s);
extern void parse_ini(const char *file);

////// compile.c : Compilation and linking /////
extern int compute_size_all(); // Size computation phase
extern void compile_all(); // Compilation phase
extern void link_all(); // Link phase

////// output.c : Binary output and image generation //////

extern FILE *imgout;
extern char output[256];
extern void out_cdimage(); // Output the whole CD image
extern void out_genimage(); // Output the whole genesis image
extern void fix_genimage(); // Fix checksum and ROM size fields in the header and enlarge to next power of two


////// d68k.c : m68k disassembler

extern int d68k(char *target, int tsize, const u8 *code, int size, sv_t address, int labels, int *suspicious);
extern const char * d68k_error(int r);


////// symtable.c

extern void setdsym(const char *name, u32 val);
extern u32 * getdsym(const char *name);
extern const char * getdsymat(u32 val); // returns the first debug symbol matching val
extern int parsedsym(const char *text);
extern int parsedsymfile(const char *name);

////// bdp.c

// Register offsets
#define REG_D(n) (REGADDR + (n) * 4)
#define REG_A(n) (REG_D(8) + (n) * 4)
#define REG_SP REG_A(7)
#define REG_PC REG_A(8)
#define REG_SR REG_A(9)

// Genesis command constants
#define CMD_HANDSHAKE   0x00
#define CMD_EXIT        0x20
#define CMD_READ        0x00
#define CMD_WRITE       0x20
#define CMD_BYTE        0x40
#define CMD_WORD        0xC0
#define CMD_LONG        0x80
#define CMD_LEN         0x1F

extern int genfd;
extern u8 inp[40];
extern int inpl;
extern int bdpdump;
extern int readdata();
extern void (*senddata)(const u8 *data, int size);
extern void sendcmd(u8 header, u32 address);
extern void sendbyte(u32 value);
extern void sendword(u32 value);
extern void sendlong(u32 value);
extern void subreq();
extern void subsetbank(int bank);
extern void subrelease();
extern void subreset();
extern void readburst(u8 *target, u32 address, int length);
extern u32 readlong(u32 address);
extern u32 readword(u32 address);
extern u32 readbyte(u32 address);
extern void readmem(u8 *target, u32 address, int length);
extern void subreadmem(u8 *target, u32 address, int length);
extern void sendburst(u32 address, const u8 *source, int length);
extern void sendmem(u32 address, const u8 *source, int length);
extern void sendmem_verify(u32 address, const u8 *source, int length);
extern void writelong(u32 address, u32 val);
extern void writeword(u32 address, u32 val);
extern void writebyte(u32 address, u32 val);
extern void subwritelong(u32 address, u32 l);
extern void subwriteword(u32 address, u32 w);
extern void subwritebyte(u32 address, u32 b);
extern void subsendmem(u32 address, const u8 *source, int length);
extern void subsendmem_verify(u32 address, const u8 *source, int length);
extern void vdpsetreg(int reg, u8 value);
extern void vramsendmem(u32 address, const u8 *source, int length);
extern void vramreadmem(u8 *target, u32 address, int length);
extern void sendfile(const char *file, u32 address);
extern void subsendfile(const char *path, u32 address);
extern void open_debugger(const char *path);

////// binparse.c


// Returns the type of the image
// 0 = invalid
// 1 = genesis
// 2 = CD
extern int getimgtype(const u8 *img, int size);

// Returns the offset and size of IP (skipping header and security code)
extern int getipoffset(const u8 *img, const u8 **out_ip_start);
extern int getspoffset(const u8 *img, const u8 **out_sp_start);

// Returns security code length
extern int detect_region(const u8 *img);

#endif//BLSBUILD_H
