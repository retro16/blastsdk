#ifndef BEH_H
#define BEH_H

#if TARGET == TARGET_SCD1 || TARGET == TARGET_SCD2
extern void BEH_INIT_SCD_VECTORS();
static inline void beh_init() { BEH_INIT_SCD_VECTORS(); }
#else
static inline void beh_init() { }
#endif

#endif
