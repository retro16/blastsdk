#ifndef BEH_H
#define BEH_H

#if TARGET == TARGET_SCD1 || TARGET == TARGET_SCD2
#define BEH_INIT BEH_INIT_SCD_VECTORS
extern void BEH_INIT_SCD_VECTORS();
#else
#define BEH_INIT() do {} while(0)
#endif

#endif
