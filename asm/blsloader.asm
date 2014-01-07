; Blast! SDK Genesis loader routines

		if SCD != 0
		include	blsloader_cd.asm
		fi

;;;;;
; blsload_cart_mram_raw(void *dest, void *src, unsigned int len);
; Copies from cartridge to main RAM
blsload_cart_ram_raw	equ	memcpy

;;;;;
; blsload_cart_zram_raw(void *dest, void *src, unsigned int len);
; Copies from cartridge to Z80 RAM
blsload_cart_zram_raw	equ	memcpy

;;;;;
; blsload_cart_vram_raw(void *dest, void *src, unsigned int len);
; Copies from cartridge to video RAM
blsload_cart_vram_raw	equ	blsvdp_dma

