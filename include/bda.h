#ifndef BLS_BDA_H
#define BLS_BDA_H

#if BUS == BUS_MAIN
extern void BDA_INIT();
static inline void bda_init() { BDA_INIT(); }
#endif

#endif
