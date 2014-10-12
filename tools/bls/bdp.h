#ifndef BDP_H
#define BDP_H

typedef void (*unknown_handler_ptr)(const u8 inp[36], int inpl);
typedef void (*breakpoint_handler_ptr)(int cpu, u32 address);
typedef void (*exception_handler_ptr)(int cpu, int ex);

extern int genfd; // Genesis file descriptor

// Device access
extern void open_debugger(const char *path, unknown_handler_ptr on_unknown, breakpoint_handler_ptr on_breakpoint, exception_handler_ptr on_exception);
extern void close_debugger();
extern void bdp_set_dump(int newdump);
extern void bdp_readdata();
extern void bdp_readbuffer();
extern int bridgequery(u8 *data);
extern void update_bda_sub(const u8 *image, int imgsize);

// Memory read
extern void readmem(int cpu, u8 *target, u32 address, u32 length);
extern void readwram(int mode, const u8 *source, u32 length); // Mode : 0 = 2M, 1 = 1M bank 0, 2 = 1M bank 1
extern void readvram(u8 *target, u32 address, u32 length);

// Memory write
extern void writemem(int cpu, u32 address, const u8 *source, u32 length);
extern void writemem_verify(int cpu, u32 address, const u8 *source, u32 length);
extern void sendfile(int cpu, const char *filename, u32 address);
extern void writewram(int mode, u32 address, const u8 *source, u32 length);
extern void writevram(u32 address, const u8 *source, u32 length);

// Bus access
extern u32 readlong(int cpu, u32 address);
extern u32 readword(int cpu, u32 address);
extern u32 readbyte(int cpu, u32 address);
extern void writebyte(int cpu, u32 address, u32 b);
extern void writeword(int cpu, u32 address, u32 w);
extern void writelong(int cpu, u32 address, u32 l);

// CPU access
extern void subfreeze();
extern void dbgstatus(); // To be removed
extern u32 readreg(int cpu, int reg); // 0-7 = D0-D7, 8-15 = A0-A7, 16 = PC, 17 = SR
extern void readregs(int cpu, u32 *regs); // 0-7 = D0-D7, 8-15 = A0-A7, 16 = PC, 17 = SR
extern void setreg(int cpu, int reg, u32 value);
extern void setregs(int cpu, u32 *regs);
extern void startcpu(int cpu); // Enter run mode
extern void stopcpu(int cpu); // Enter monitor mode
extern void stepcpu(int cpu); // Execute only one instruction
extern void reach_breakpoint(int cpu); // Run until a breakpoint is reached
extern void resetcpu(int cpu);
extern int is_running(int cpu);

// VDP access
extern void vdpsetreg(int reg, u8 value);

// Breakpoints
extern void setup_breakpoints(int cpu);
extern void cleanup_breakpoints(int cpu);
extern void list_breakpoints(int cpu);
extern int has_breakpoint(int cpu, u32 address);
extern void set_breakpoint(int cpu, u32 address);
extern int delete_breakpoint(int cpu, u32 address);

#endif
// vim: ts=2 sw=2 sts=2 et
