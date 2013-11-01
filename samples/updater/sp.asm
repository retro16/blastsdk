		include	bls.inc
		include cdbios.inc

SP_INIT
		BIOS_CDCSTOP			; Stop CD-ROM drive
		rts

SP_MAIN
		bra.b	SP_MAIN

		ds	30			; Make room for code upload
INT_SUB_LEVEL2
		rts
