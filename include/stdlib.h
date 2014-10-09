#ifndef STDLIB_H
#define STDLIB_H

#if BUS == BUS_MAIN
extern void _MAIN_MEMCPY(char *dest, const char *src, u32 size;
static inline void memcpy(char *dest, const char *src, u32 size) {
  _MAIN_MEMCPY(dest, src, size);
}
extern void _MAIN_MEMSET(char *dest, char data, u32 size;
static inline void memset(char *dest, char data, u32 size) {
  _MAIN_MEMSET(dest, data, size);
}
#elif BUS == BUS_SUB
extern void _SUB_MEMCPY(char *dest, const char *src, u32 size;
static inline void memcpy(char *dest, const char *src, u32 size) {
  _SUB_MEMCPY(dest, src, size);
}
extern void _SUB_MEMSET(char *dest, char data, u32 size;
static inline void memset(char *dest, char data, u32 size) {
  _SUB_MEMSET(dest, data, size);
}
#endif

#endif
