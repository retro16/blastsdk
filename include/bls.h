/* Blast! SDK main include
 */


/* blsvdp.asm */
extern void BLSVDP_DMA(void *dest, void *src, unsigned int len);
extern void BLSVDP_DMA_CRAM(void *dest, void *src, unsigned int len);
extern void BLSVDP_PREPARE_INIT(short *prepared);
extern void BLSVDP_PREPARE_DMA(void *dest, void *src, unsigned int len, short *prepared);
extern void BLSVDP_EXEC(short *prepared);

