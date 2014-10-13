Widescreen display mode library.

This library displays a widescreen resolution framebuffer (256x152) at 60 FPS.
It uses the extended vblank trick to allow quick DMA copies.

The framebuffer is displayed on plane A with low priority tiles. Planes are set
to 64x32.

To use, include wscr.md in your main cpu binary and wscr_sub.md in your sub cpu
binary.


### On the main CPU

Instead of calling bls_init_vdp like you would do normally, call wscr_init_vdp.



### On the sub CPU


---------------------------------------

 - provides wscr.asm wscr_fbuf_plane wscr_fbuf_chr


Section wscr_fbuf_plane
=======================

This section stores framebuffer plane data.

 - format empty
 - chip vram
 - size $640 (enough to display a 320x160 buffer)
 - align $2000


Section wscr_hscroll
====================

This section stores hscroll data.

 - format empty
 - chip vram
 - size 4
 - align $400


Section wscr_fbuf_chr
=====================

This section stores framebuffer character images. Enough for a 240x160 display
plus one null tile.

 - format empty
 - chip vram
 - size $4B20
 - align $20

