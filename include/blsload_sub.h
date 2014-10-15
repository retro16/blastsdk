#ifndef BLSLOAD_SUB_H
#define BLSLOAD_SUB_H

// Check if the main CPU made a load request
// You should call this at each level 2 interrupt
extern void BLSLOAD_CHECK_MAIN();
static inline void blsload_check_main() { BLSLOAD_CHECK_MAIN(); }

// Start buffering <count> sectors from <sector>
extern void BLSLOAD_START_READ(u32 sector, u32 count);
static inline void blsload_start_read(u32 sector, u32 count) { BLSLOAD_START_READ(sector, count); }

// Stream <bytes> bytes from CD buffer to <target>
// <bytes> must be even and <target> must be word aligned
extern void BLSLOAD_READ_CD(u32 bytes, u32 target);
static inline void blsload_read_cd(u32 bytes, u32 target) { BLSLOAD_READ_CD(bytes, target); }

#endif
