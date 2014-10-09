#ifndef BLS_BDP_H
#define BLS_BDP_H


extern void _MAIN_BDP_WRITE(const char *data, int size);
static inline void bdp_write(const char *data, int size) { _MAIN_BDP_WRITE(data, size); }

#if TARGET == TARGET_SCD1 || TARGET == TARGET_SCD2
extern void BDP_SUB_CHECK();
static inline void bdp_sub_check() { BDP_SUB_CHECK(); }
#else
static inline void bdp_sub_check() { }
#endif

#endif
