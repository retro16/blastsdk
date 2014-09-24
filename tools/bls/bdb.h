#ifndef BDB_H
#define BDB_H
#include <stdint.h>

#include "bls.h"

////// Types

typedef struct dsym {
  struct dsym *next;
  char name[NAMELEN];
  u32 val;
} *dsymptr;

////// Globals

extern dsymptr dsymtable;


////// Declarations

// d68k.c
extern int d68k(char *target, int tsize, const u8 *code, int size, u32 address, int labels, int *suspicious);
extern const char *d68k_error(int code);

// symtable.c
extern void setdsym(const char *name, u32 val);
extern u32 *getdsym(const char *name);
extern const char *getdsymat(u32 val);  // returns the first debug symbol matching val
extern int parsedsym(const char *text);
extern int parsedsymfile(const char *name);

// bdp.c

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
extern int readdata();
extern void senddata(const u8 *data, int size);
extern void sendcmd(u8 header, u32 address);
extern void sendbyte(u32 value);
extern void sendword(u32 value);
extern void sendlong(u32 value);
extern void readburst(u8 *target, u32 address, int length);
extern u32 readlong(u32 address);
extern u32 readword(u32 address);
extern u32 readbyte(u32 address);
extern void readmem(u8 *target, u32 address, int length);
extern void subreadmem(u8 *target, u32 address, int length);
extern void sendburst(u32 address, const u8 *source, int length);
extern void sendmem(u32 address, const u8 *source, int length);
extern void writelong(u32 address, u32 val);
extern void writeword(u32 address, u32 val);
extern void writebyte(u32 address, u32 val);
extern void subsendmem(u32 address, const u8 *source, int length);
extern void sendfile(const char *file, u32 address);
extern void subsendfile(const char *path, u32 address);
extern void open_debugger(const char *path);
#endif

// vim: ts=2 sw=2 sts=2 et
