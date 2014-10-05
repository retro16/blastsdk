#ifndef BLSLOAD_SUB_H
#define BLSLOAD_SUB_H

// Start buffering <count> sectors from <sector>
extern void BLSLOAD_START_READ(u32 sector, u32 count);

// Stream <bytes> bytes from CDD buffer to <target>
// <bytes> must be even and <target> must be word aligned
extern void BLSLOAD_READ_CD(u32 bytes, u32 target);

#endif
