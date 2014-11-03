                include bls_init.inc
                include bls_vdp.inc
                include bdp.inc
                include sub_idle.inc

MAIN
IP_MAIN
                bls_init 0, INT_VBLANK
                sub_idle_init
                bls_init_vdp 0, 1, 64, 32, BLAST_SPLASH_PNG_MAP, 0, PLANE_B, SPRAT, HSCROLL_TABLE, 0, 0, 0, 0, 0, 0, 0

                BLSLOAD_BINARY_BLAST_SPLASH

                delay_millis 3000

                BLSLOAD_BINARY_TEXT
                ccall display_text
.1 bra.b .1

INT_VBLANK
                movem.l d0/d1/a0/a1, -(sp)
                sub_idle_vsync
                movem.l (sp)+, d0/d1/a0/a1
                rte


; vim: ts=8 sw=8 sts=8 et
