#define BLSCDR_COMM GA_COMMWORD_MAIN_L(3)
#define BLSCDR_FLAG 0x20
#define BLSCDR_BIT 5

static inline void sync_main_sub() {
#if BUS == BUS_MAIN
  u8 current_state = *GA_COMMFLAGS_SUB & 0x10;
  while((*GA_COMMFLAGS_SUB & 0x10) == current_state);
  *GA_COMMFLAGS_MAIN ^= 0x10;
#else
  u8 current_state = *GA_COMMFLAGS_MAIN & 0x10;
  while((*GA_COMMFLAGS_MAIN & 0x10) == current_state) {
    *GA_COMMFLAGS_SUB ^= 0x10;
  }
#endif
}

#if BUS == BUS_MAIN

static inline void sub_busreq() {
  *GA_RH_BL = GA_NORESET | GA_BUSREQ;
  while(!(*GA_RH_BL & GA_BUSREQ));
}

static inline void sub_busrelease() {
  *GA_RH_BL = GA_NORESET;
}

static inline void sub_reset() {
  *GA_RH_BL = GA_NORESET;
  *GA_RH_BL = 0;
  *GA_RH_BL = 0;
  *GA_RH_BL = GA_NORESET;
}

static inline void sub_set_bank(u8 bank) {
  *GA_MM = ((*GA_MM >> 1) & GA_DMNA) | (bank << 6);
}

static inline u8 sub_access_pram(u8 bank) {
  u8 oldmm = *GA_MM;
  u16 oldstate = *GA_RH_BL | (oldmm << 8);
  *GA_MM = ((oldmm >> 1) & GA_DMNA) | (bank << 6);
  return oldstate;
}

static inline void sub_release_pram(u16 oldstate) {
  u8 oldmm = oldstate >> 8;
  *GA_RH_BL = (u8)oldstate;
  *GA_MM = ((oldmm >> 1) & GA_DMNA) | (oldmm & 0xC0);
}

static inline void sub_interrupt() {
  *GA_RH |= GA_IFL2_BH;
}

#else

static inline void sub_int_disable() {
  bls_disable_interrupts();
  *GA_IMASK_BL = 0;
}

static inline void sub_int_enable() {
  bls_enable_interrupts();
  *GA_IMASK_BL = GA_IM_L1 | GA_IM_L2 |  GA_IM_L3 | GA_IM_L4 | GA_IM_L5 | GA_IM_L6;
}

#endif

static inline void send_wram_2m() {
#if BUS == BUS_MAIN
  *GA_MM |= GA_DMNA;
  while(!(*GA_MM & GA_DMNA));
#else
  *GA_MM |= GA_RET;
  while(!(*GA_MM & GA_RET));
#endif
}

static inline void wait_wram_2m() {
#if BUS == BUS_MAIN
  while(!(*GA_MM & GA_RET));
#else
  while(*GA_MM & GA_RET);
#endif
}

static inline u8 has_wram_2m() {
#if BUS == BUS_MAIN
  return *GA_MM & GA_RET;
#else
  return !(*GA_MM & GA_RET);
#endif
}

#if BUS == BUS_SUB
static inline void swap_wram_1m() {
  *GA_MM ^= GA_RET;
  while(*GA_MM & GA_DMNA);
}
#endif

static inline void swap_sync_1m() {
#if BUS == BUS_MAIN
  *GA_MM |= GA_DMNA;
  while(*GA_MM & GA_DMNA);
#else
  swap_wram_1m();
#endif
}

#if BUS == BUS_MAIN
static inline void swap_req_1m() {
  *GA_MM |= GA_DMNA;
}

static inline void swap_wait_1m() {
  while(*GA_MM & GA_DMNA);
}

static inline void blsload_start_read(u32 sector, u8 count) {
  *BLSCDR_COMM = sector | ((u32)count << 24);
  GA_COMMFLAGS_MAIN |= BLSCDR_FLAG;
#if TARGET == TARGET_SCD1
  swap_req_1m();
#else
  if(has_wram_2m()) {
    send_wram_2m();
  }
#endif
}
#endif

static inline void blsload_wait_read() {
#if TARGET == TARGET_SCD1
  swap_wait_1m();
#else
  wait_wram_2m();
#endif
}

static inline u8 blsload_read_ready() {
#if TARGET == TARGET_SCD1
  return !(*GA_MM & GA_DMNA);
#else
  return (*GA_MM & GA_RET);
#endif
}

static inline void blsload_read_cd(u32 sector, u8 count) {
  blsload_start_read(sector, count);
  blsload_wait_read();
}
