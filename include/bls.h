/* Blast! SDK main include
 */


/* blsvdp.asm */
extern void blsvdp_dma(void *dest, void *src, unsigned int len);
extern void blsvdp_prepare_init(short *prepared);
extern void blsvdp_prepare_dma(void *dest, void *src, unsigned int len, short *prepared);
extern void blsvdp_exec(short *prepared);

