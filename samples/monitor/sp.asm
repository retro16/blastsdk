		include	bls.inc
		include cdbios.inc

SP_INIT
		BIOS_CDCSTOP			; Stop CD-ROM drive
.1	bra.b	.1
	jsr $0200.l
		rts

SP_MAIN
		bra.b	SP_MAIN

		ds	30			; Make room for code upload
G_INT_SUB_LEVEL2
		rts
