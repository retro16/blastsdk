Widescreen display mode library.

This library displays a widescreen resolution framebuffer (256x152) at 60 FPS.
It uses the extended vblank trick to allow quick DMA copies. For now, it only
supports streaming from the 2M mode framebuffer (the scale/rotation feature of
the sub CPU).

The framebuffer is displayed on plane B. Planes are set to 64x32 cells.

Also, commword 0 and 1 store the status of gamepads 1 and 2, respectively, and
commword 2 contains the number of skipped frames, that is the number of times
vblank was reached but WRAM was not available for main CPU.

To use, include wscr.md in your main cpu binary and wscr_sub.md in your sub cpu
binary.

2 callbacks are provided: one called if WRAM is available to the main CPU and
the other one if WRAM is not available (vblank miss).

The callback must return non-0 in d0.b ("return 1" in C) to enable the DMA copy
from WRAM to VRAM. Beware that the code must be very quick (100-200 cycles, no
more) or the routine will have not enough time to do the DMA. Of course, if the
callback chooses not to do the DMA copy, more time will be available (somewhere
near 40000 cycles).

During callbacks, a4 points to VDPDATA and a5 to VDPCTRL (you can use
VDPUSEREG).

The vblank miss callback has no return value.

Note: The level2 interrupt will not be issued to the sub CPU in case of a
vblank miss.


### On the main CPU

Instead of calling bls_init_vdp like you would do normally, call wscr_init_vdp.
This function will disable normal vblank interrupt and setup its own hblank
interrupt that will increase vblank duration. This interrupt will test if WRAM
is available, send the framebuffer to screen if needed, poll the gamepad and
update the skipped frame counter.


### On the sub CPU

Just generate an image in the framebuffer and give WRAM back to main CPU.

To account for skipped frames, you may use this main loop algorithm:

    while(true) {
        for(frame = 0; frame < *wscr_miss_count; ++frame) {
            do_gameplay();
        }
        render_graphics();
        send_wram_2m();
        wait_vram_2m();
    }


---------------------------------------

 - provides wscr.asm wscr_fbuf_plane wscr_fbuf_chr


Section wscr_fbuf_plane
=======================

This section stores framebuffer plane data.

 - format empty
 - chip vram
 - size $5F0 (enough to display a 320x152 buffer)
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

Section wscr_framebuffer
========================

This section stores the framebuffer in word RAM

 - format empty
 - chip wram
 - size 19456 (256*152/2)
 - align $20
