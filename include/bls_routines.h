#ifndef BLS_ROUTINES_H
#define BLS_ROUTINES_H

// Purely inline fill function.
// Optimized for constant parameters
static inline void blsfastfill(void *dest, u8 value, u32 bytes) {
  u8 *db = (u8 *)dest;
  if((u32)db & 1) {
    *db = value;
    --bytes;
  }
  u32 *dl = (u32 *)db;
  u32 qval = (((u32)value) << 24) | (((u32)value) << 16) | (((u32)value) << 8) | value;
  for(; bytes >= 4; bytes -= 4) {
    *dl = qval;
    ++dl;
  }
  db = (u8 *)dl;
  while(bytes--) {
    *db = value;
    ++db;
  }
}


// Purely inline copy function.
// Optimized for constant parameters
static inline void blsfastcopy_aligned(void *dest, void *src, u32 bytes) {
  u8 *d = (u8 *)dest;
  u8 *s = (u8 *)src;
  if(!((u32)d & 1) && !((u32)s & 1)) {
    // Aligned pointers
    u32 *dl = (u32 *)dest;
    u32 *sl = (u32 *)src;

    for(; bytes >= 4; bytes -= 4) {
      *dl = *sl;
      ++dl;
      ++sl;
    }
    d = (u8 *)dl;
    s = (u8 *)sl;
  }

  while(bytes--) {
    *d = *s;
    ++d;
    ++s;
  }
}

static inline void bls_setintvector(bls_int_vector vector, bls_int_callback target) {
#if TARGET != TARGET_GEN
#if TARGET == TARGET_SCD1 || TARGET == TARGET_SCD2
  if(vector == INTV_HBLANK) {
    // Special case for hblank on the sega CD
    *GA_HINT = (u16)target;
    return;
  }
#endif
  if(*vector != target) {
    u8 *vtarget = (u8 *)*vector; // Current vector target

    u16 *jmp = (u16 *)vtarget;
    u32 *jmptarget = (u32 *)(vtarget + 2);

    *jmp = 0x4EF9; // Write JMP ###.L instruction
    *jmptarget = (u32)target; // Write JMP target address
  }
#endif
}

static inline void bls_enable_interrupts() {
  asm volatile("\tandi #0xF8FF, %sr\n");
}

static inline void bls_disable_interrupts() {
  asm volatile("\tori #0x0700, %sr\n");
}

static inline void trap(t) {
#define TRAP_STRING(t) #t
  asm volatile("\tTRAP #" TRAP_STRING(t));
#undef TRAP_STRING
}

static inline void enter_monitor() {
  trap(7);
}

#endif
