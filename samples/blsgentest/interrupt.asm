int_level6
		btst	#CUP_BIT, CDATA1		; Test up button
		beq.b	go_up
		btst	#CDOWN_BIT, CDATA1
		beq.b	go_down
interrupt	rte

go_up
		addi.w	#1, vscroll_a
		bra.b	upload_vscroll
go_down
		subi.w	#1, vscroll_a

int_trap00
upload_vscroll
		VdpUseAddr
		VdpSetWriteVScroll 0
		move.w	vscroll_a, VDPDATA
		rte
