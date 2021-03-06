		include bls_init.inc

        if TARGET == TARGET_SCD1 || TARGET == TARGET_SCD2
IP_MAIN
                SYNC_MAIN_SUB
        endif

MAIN
                BLS_INIT 0, INT_VBLANK

                VDPSETADDRREG                   ; Access VDP through a4/a5

		VDPDMASEND FONT_PNG_IMG, 0, FONT_PNG_IMG_SIZE, VRAM ; Upload font tiles

		VDPSETWRITE 0, CRAM
		move.l	#$000008AA, (a4)	; Set background and text color
		VDPSETWRITE 66, CRAM		; Point at palette 3, index 1
                move.w  #$0EC8, (a4)            ; Set odd column text color
		VDPSETWRITE 98, CRAM		; Point at palette 4, index 1
                move.w  #$0EEC, (a4)            ; Set highlighted odd column text color
		VDPSETWRITE 34, CRAM		; Point at palette 2, index 1
		move.w	#$04E8, (a4)	        ; Set highlighted text color

		; Initialize RAM values
		move.l	#$FF0000, dumpscroll
		move.l	#$FF0000, addr
		move.l	#$00000000, value
		move.b	#1, mode
		move.w	#0, cursor

                BLS_INIT_VDP 0, 1, 64, 32, PLANE_A_DEF, 0, PLANE_B_DEF, SPRAT_DEF, HSCROLL_TABLE_DEF, VDPHSCRSCROLL, VDPVSCRSCROLL, 0, 0, VDPWIN_TOPLEFT, 0, 0

.mainloop       bra.b   .mainloop



; Screen line display example : FF0004 DEADBEEF 4
;
; Controls : Left / Right : select nybble
;            Up / Down : change nybble
;            B : read
;            C : write
;
; Middle character is 1 for byte, 2 for word, 4 for long and B for branch

printline
		VDPSETWRITEPLANE PLANE_A_DEF, 64, 0, 0
		move.w	#0, (a4)	; Print space
		move.w	#0, (a4)	; Print space
		move.w	#0, (a4)	; Print space
		move.w	#0, (a4)	; Print space

		move.l	addr, a0
		bsr.w	printaddr	; Print the current address

		move.w	#0, (a4)	; Print space

		move.l	value, d0
		bsr.w	printlong	; Print the current value

		move.w	#0, (a4)	; Print space

		move.b	mode, d7	; Read mode
		andi.w	#$3, d7		; Filter value
		lsl.w	#1, d7		; Table is made of words
		lea	mode_table, a0	; Transform mode to readable value using mode_table
		move.w	(a0, d7), (a4)	; Print mode character

		moveq	#(40-21-1), d0	; Fill the rest of the line with space
.1		move.w	#0, (a4)
		dbra	d0, .1

hilight_cursor
		move.w  #PLANE_A_DEF, d2
		move.b  cursor+1, d2	; Character number to d2
		addq	#8, d2
		VDPSETREADVAR d2, VRAM	; Read from VRAM at highlighted character
		move.w	(a4), d1	; Read character
		ori.w	#$2000, d1	; Set palette 1
		VDPSETWRITEVAR d2, VRAM	; Write to VRAM at highlighted character
		move.w	d1, (a4)	; Replace highlighted character
dump_ram
		move.l	dumpscroll, a0
		move.w	#PLANE_A_DEF + 64*2, d4	; VRAM address of the beginning of the line
		moveq	#26, d6		; Line count

.1		VDPSETWRITEVAR d4, VRAM
		bsr.b	printaddr	; Print address on the first column
                move.w  #0, (a4)        ; Separate address and data with a space

		moveq	#3, d5		; Longs per line
.2              cmpi.w  #1, d5          ; Leave a space column at the center of the screen
                bne.b   .3
  		move.w	#0, (a4)	; Separate value with a space
.3		move.l	(a0)+, d0	; Dump one RAM word
		bsr.b	printlong	; Display data on screen
		dbra	d5, .2

		move.w	#0, (a4)	; Print spaces on the right
		move.w	#0, (a4)	; 

		addi.w	#64*2, d4	; Skip to next display line
		dbra	d6, .1
		rts

byteattr        dw      0               ; Byte display attribute

; Print the byte in d0 as hexadecimal
; Destroys d0-d1
printbyte
		move.b	d0, d1
		lsr.b	#4, d1
		andi.w	#$000F, d1
		addq.b	#1, d1
                or.w    byteattr, d1
		move.w	d1, (a4)
		andi.w	#$000F, d0
		addq.b	#1, d0
                or.w    byteattr, d0
		move.w	d0, (a4)
		rts

; Print the word in d0
; Destroys d0-d2
printword
		move.b	d0, d2
		lsr.w	#8, d0
		bsr.b	printbyte
		move.b	d2, d0
		bra.b	printbyte

; Print the address in a0
printaddr
		movem.l	a0, -(a7)
		movem.w	(a7)+, d0
		bsr.b	printbyte
		movem.w	(a7)+, d0
		bra.b	printword

; Print the long word in d0
printlong
                move.w  #$4000, byteattr
		movem.l	d0, -(a7)
		movem.w	(a7)+, d0
		bsr.b	printword
                clr.w   byteattr
		movem.w	(a7)+, d0
		bra.b	printword

int_vblank
		if TARGET == TARGET_SCD1 || TARGET == TARGET_SCD2
		SUB_INTERRUPT		; Trigger L2 interrupt on the SCD
		endif

		movem.l	d0-d7/a0-a6, -(a7)
		lea	VDPDATA, a4
		lea	VDPCTRL, a5

		move.b	15*4(a7), d0
		cmp.b	#$20, d0
		beq.b	.nospurious
		VDPSETBG #$750
.nospurious

		VDPSETREAD $0, CRAM
		move.w	(a4), d0
		lsr.w	#1, d0
		andi.w	#$666, d0
		VDPSETWRITE $0, CRAM
		move.w	d0, (a4)

		VDPSETREG 15, 2			; Reset auto-increment

		; Input detection
		lea	CDATA1, a0
		move.b	#0, (a0)		; Read with CSEL clear (A / Start)
		clr.l	d0
		nop
		nop
		move.b	(a0), d0
		move.b	#CSEL, (a0)		; Read with CSEL set (other buttons)
		lsl.w	#8, d0
		move.b	(a0), d0
		cmp.w	lastinput, d0		; If input changed
		bne.b	newinput		; Process new input

		subq.b	#1, inrepeat		; Auto-repeat delay
		bne.w	int_vblank_end		; Auto-repeat blocking

		; Auto-repeat reached 0 : reload and process input
		move.b	#4, inrepeat		; Fast repeat on hold
		bra.b	processinput

		; Process input
newinput
		move.b	#25, inrepeat		; Slow repeat on new input
processinput
		btst	#CBTNSTART_BIT+8, d0	; Test start
		bne.b	.nostart

		; Start pressed : scroll view
		btst	#CUP_BIT, d0
		bne.b	.noup
		subi.l	#16, dumpscroll		; Scroll one line up

.noup		btst	#CDOWN_BIT, d0
		bne.b	.nodown
		addi.l	#16, dumpscroll		; Scroll one line down

.nodown		btst	#CLEFT_BIT, d0
		bne.b	.noleft
		subi.l	#(16*27), dumpscroll	; Scroll one page up

.noleft		btst	#CRIGHT_BIT, d0
		bne.b	int_vblank_end
		addi.l	#(16*27), dumpscroll	; Scroll one page down

		bra.b	int_vblank_end

		; Start not pressed : normal actions
.nostart	btst	#CUP_BIT, d0
		beq.b	pressup
		btst	#CDOWN_BIT, d0
		beq.b	pressdown
		btst	#CLEFT_BIT, d0
		beq.w	pressleft
		btst	#CRIGHT_BIT, d0
		beq.w	pressright
		btst	#CBTNB_BIT, d0		; Test B
		beq.w	presswrite
		btst	#CBTNC_BIT, d0		; Test C
		beq.w	pressread
		btst	#CBTNA_BIT+8, d0		; Test 1
		bne.b	int_vblank_end
		move.l	addr, dumpscroll

int_vblank_end
		move.w	d0, lastinput		; Store current input value until next frame
		bsr.w	printline		; Refresh display
		movem.l	(a7)+, d0-d7/a0-a6
		rte

interrupt
		VDPSETBG #$F0F			; Magenta background = unknown interrupt
		rte

pressup
		lea	addr, a0
		lea	cursor_offset_table, a1
		move.w	cursor, d4		; Compute offset table index
		moveq	#0, d1
		move.b	(a1, d4), d1		; Read byte offset
		move.b	1(a1, d4), d2		; Read bit offset
		moveq	#1, d3
		lsl.b	d2, d3			; Compute delta
		add.b	d3, (a0, d1)		; Add value to target
		bra.b	int_vblank_end

pressdown
		lea	addr, a0
		lea	cursor_offset_table, a1
		move.w	cursor, d4		; Compute offset table index
		moveq	#0, d1
		move.b	(a1, d4), d1		; Read byte offset
		move.b	1(a1, d4), d2		; Read bit offset
		moveq	#1, d3
		lsl.b	d2, d3			; Compute delta
		sub.b	d3, (a0, d1)		; Sub value from target
		bra.b	int_vblank_end

pressleft
		move.w	cursor, d1
		subq.b	#2, d1
		bcc.b	.1
		moveq	#32, d1

.1		cmp.b	#12, d1
		bne.b	.2
		moveq	#10, d1

.2		cmp.b	#30, d1
		bne.b	.3
		moveq	#28, d1

.3		move.w	d1, cursor
		bra.w	int_vblank_end

pressright
		move.w	cursor, d1
		addq.b	#2, d1

		cmp.b	#34, d1
		bne.b	.1
		moveq	#0, d1

.1		cmp.b	#12, d1
		bne.b	.2
		moveq	#14, d1

.2		cmp.b	#30, d1
		bne.b	.3
		moveq	#32, d1

.3		move.w	d1, cursor
		bra.w	int_vblank_end

pressread
		VDPSETBG #$0C0			; Green background = read
		move.l	addr, d2
		move.b	mode, d1
		btst	#1, d1
		bne.b	.2
		btst	#0, d1
		bne.b	.1

		; mode == 0 : byte
		move.l	d2, a0
		move.b	(a0), d1
		ext.w		d1
		ext.l		d1
		move.l	d1, value
		bra.w	int_vblank_end

; XXX to be optimized

.1		; mode == 1 : word
		andi.l	#$00FFFFFE, d2
		move.l	d2, addr
		move.l	d2, a0
		move.w	(a0), d1
		ext.l		d1
		move.l	d1, value
		bra.w	int_vblank_end

.2		btst	#0, d1
		bne.b	.3

		; mode == 2 : long
		andi.l	#$00FFFFFE, d2
		move.l	d2, addr
		move.l	d2, a0
		move.l	(a0), value
		bra.w	int_vblank_end

.3		; mode == 3 : jsr
		andi.l	#$00FFFFFE, d2
		move.l	d2, addr
		move.l	d2, a0
		movem.l	d0-d7/a0-a6, -(a7)
		jsr	addr
		movem.l	(a7)+, d0-d7/a0-a6
		bra.w	int_vblank_end

presswrite
		VDPSETBG #$00C			; Red background = write
		move.l	addr, d2
		move.b	mode, d1
		btst	#1, d1
		bne.b	.2
		btst	#0, d1
		bne.b	.1

		; mode == 0 : byte
		move.l	d2, a0
		move.l	value, d1
		move.b	d1, (a0)
		bra.w	int_vblank_end

.1		; mode == 1 : word
		andi.l	#$00FFFFFE, d2
		move.l	d2, addr
		move.l	d2, a0
		move.l	value, d1
		move.w	d1, (a0)
		bra.w	int_vblank_end

.2		btst	#0, d1
		bne.b	.3

		; mode == 2 : long
		andi.l	#$00FFFFFE, d2
		move.l	d2, addr
		move.l	d2, a0
		move.l	value, (a0)
		bra.w	int_vblank_end

.3		; mode == 3 : jsr
		andi.l	#$00FFFFFE, d2
		move.l	d2, addr
		move.l	d2, a0
		movem.l	d0-d7/a0-a6, -(a7)
		jsr	addr
		movem.l	(a7)+, d0-d7/a0-a6
		bra.w	int_vblank_end

cursor_offset_table
		db	1, 4
		db	1, 0
		db	2, 4
		db	2, 0
		db	3, 4
		db	3, 0
		db	0, 0
		db	4, 4
		db	4, 0
		db	5, 4
		db	5, 0
		db	6, 4
		db	6, 0
		db	7, 4
		db	7, 0
		db	0, 0
		db	(mode - addr), 0

mode_table
		dw	$2
		dw	$3
		dw	$5
		dw	$C

; vim: ts=8 sw=8 sts=8 et
