		include	beh.inc
		
g_int_level6
		btst	#BCUP, CDATA1		; Test up button
		beq.b	go_up
		btst	#BCDOWN, CDATA1
		beq.b	go_down
g_interrupt	rte

go_up
		addi.w	#1, vscroll_a
		bra.b	upload_vscroll
go_down
		subi.w	#1, vscroll_a
upload_vscroll
		VdpUseAddr
		VdpSetWriteVScroll 0
		move.w	vscroll_a, VDPDATA
		rte
