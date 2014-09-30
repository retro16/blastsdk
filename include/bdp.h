#ifndef BLS_BDP_H
#define BLS_BDP_H

extern BDP_WRITE(const char *data, int size);

#if BUS == BUS_MAIN
#if TARGET == TARGET_SCD1 || TARGET == TARGET_SCD2
extern void BDP_SUB_CHECK();
#else
#define BDP_SUB_CHECK() do {} while(0)
#endif
#endif

#endif
