#ifndef BLS_BDP_SUB_H
#define BLS_BDP_SUB_H

extern _SUB_BDP_WRITE(const char *data, int size);
static inline void bdp_write(const char *data, int size) { _SUB_BDP_WRITE(data, size); }

#endif
