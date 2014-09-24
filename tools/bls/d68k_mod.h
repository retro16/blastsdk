/* 68000 disassembler.
 */

// Errors returned by d68k()
extern const char *d68k_error(int64_t r);
extern int64_t d68k(char *_targetdata, int _tsize, const u8 *_code, int _size, int _instructions, u32 _address, int _labels, int _showcycles, int *_suspicious);
extern void d68k_readsymbols(const char *filename);
extern void d68k_freesymbols();

// vim: ts=2 sw=2 sts=2 et
