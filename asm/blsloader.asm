; Blast! SDK Genesis loader routines

		if SCD != 0
		include	blsloader_cd.asm
		fi

;;;;;
; blsload_cart_mram(void *dest, void *src, unsigned int len);
; Copies from cartridge to main RAM
blsload_cart_mram	equ	memcpy

;;;;;
; blsload_cart_zram(void *dest, void *src, unsigned int len);
; Copies from cartridge to Z80 RAM
blsload_cart_zram	equ	memcpy

;;;;;
; blsload_cart_vram(void *dest, void *src, unsigned int len);
; Copies from cartridge to video RAM
blsload_cart_vram	equ	blsvdp_dma

