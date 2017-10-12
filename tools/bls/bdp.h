#ifndef BDP_H
#define BDP_H

// Public interface of BDP
struct bdp {
  // File descriptor
  // Call readfd(struct bdp *bdp) to flush data from the file descriptor.
  int fd;

  // Callback opaque identifier
  void *callback_value;

  // Device access
  void (*readfd)(struct bdp *bdp); // Read from the file descriptor
  void (*close)(struct bdp *bdp); // Release all resources
  void (*set_dump)(struct bdp *bdp, int newdump);

  // Memory read (uses bulk download)
  void (*readmem)(struct bdp *bdp, int cpu, u8 *target, u32 address, u32 length);
  void (*readwram)(struct bdp *bdp, int mode, const u8 *source, u32 length); // Mode : 0 = 2M, 1 = 1M bank 0, 2 = 1M bank 1
  void (*readvram)(struct bdp *bdp, u8 *target, u32 address, u32 length); // Will alter VDP state on real hardware

  // Memory write (uses bulk upload)
  void (*writemem)(struct bdp *bdp, int cpu, u32 address, const u8 *source, u32 length);
  void (*writemem_verify)(struct bdp *bdp, int cpu, u32 address, const u8 *source, u32 length);
  void (*sendfile)(struct bdp *bdp, int cpu, const char *filename, u32 address);
  void (*writewram)(struct bdp *bdp, int mode, u32 address, const u8 *source, u32 length);
  void (*writevram)(struct bdp *bdp, u32 address, const u8 *source, u32 length); // Will alter VDP state on real hardware

  // Bus access
  u32 (*readlong)(struct bdp *bdp, int cpu, u32 address);
  u32 (*readword)(struct bdp *bdp, int cpu, u32 address);
  u32 (*readbyte)(struct bdp *bdp, int cpu, u32 address);
  void (*writebyte)(struct bdp *bdp, int cpu, u32 address, u32 b);
  void (*writeword)(struct bdp *bdp, int cpu, u32 address, u32 w);
  void (*writelong)(struct bdp *bdp, int cpu, u32 address, u32 l);

  // CPU access
  void (*subfreeze)(struct bdp *bdp);
  u32 (*readreg)(struct bdp *bdp, int cpu, int reg); // 0-7 = D0-D7, 8-15 = A0-A7, 16 = PC, 17 = SR
  void (*readregs)(struct bdp *bdp, int cpu, u32 *regs); // 0-7 = D0-D7, 8-15 = A0-A7, 16 = PC, 17 = SR
  void (*setreg)(struct bdp *bdp, int cpu, int reg, u32 value);
  void (*setregs)(struct bdp *bdp, int cpu, u32 *regs);
  void (*startcpu)(struct bdp *bdp, int cpu); // Enter run mode
  void (*stopcpu)(struct bdp *bdp, int cpu); // Enter monitor mode
  void (*stepcpu)(struct bdp *bdp, int cpu); // Execute only one instruction
  void (*reach_breakpoint)(struct bdp *bdp, int cpu); // Run until a breakpoint is reached
  void (*resetcpu)(struct bdp *bdp, int cpu);
  int (*is_running)(struct bdp *bdp, int cpu);

  // VDP access
  void (*vdpsetreg)(struct bdp *bdp, int reg, u8 value);

  // Breakpoints
  void (*setup_breakpoints)(struct bdp *bdp, int cpu);
  void (*cleanup_breakpoints)(struct bdp *bdp, int cpu);
  void (*list_breakpoints)(struct bdp *bdp, int cpu);
  int (*has_breakpoint)(struct bdp *bdp, int cpu, u32 address);
  void (*set_breakpoint)(struct bdp *bdp, int cpu, u32 address);
  int (*delete_breakpoint)(struct bdp *bdp, int cpu, u32 address);
};

// Callbacks
typedef void (*unknown_handler_ptr)(struct bdp *bdp, void *callback_value, const u8 inp[], int inpl);
typedef void (*breakpoint_handler_ptr)(struct bdp *bdp, void *callback_value, int cpu, u32 address);
typedef void (*exception_handler_ptr)(struct bdp *bdp, void *callback_value, int cpu, int ex);
typedef void (*incoming_data_handler_ptr)(struct bdp *bdp, void *callback_value, const u8 data[], int datalen);

// Initialize debugger
// To release memory and resources, call bdp->close(bdp);
// path can either be a serial device ("/dev/something") or a network host ("192.168.0.6" or "arduino.localdomain")
// protocol version is sensed automatically on open
extern struct bdp* open_debugger(const char *path, void *callback_value, unknown_handler_ptr on_unknown, breakpoint_handler_ptr on_breakpoint, exception_handler_ptr on_exception, incoming_data_handler_ptr on_incoming_data);

#endif
// vim: ts=2 sw=2 sts=2 et
