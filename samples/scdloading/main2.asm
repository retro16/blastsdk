				include	bls_vdp.inc

MAIN2
				VDPSETBG #$0EE

				; Pause
				moveq	#$10, d1
				swap	d1
.w
				dbra	d1, .w

				VDPSETBG #$0E0
				BLSLOAD_BINARY_BLAST_SPLASH
.1				bra.b	.1
