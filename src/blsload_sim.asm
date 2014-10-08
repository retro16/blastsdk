; Injected by bdb into the sub cpu bios.

; Compile with :
;  asmx -w -e -b 0xA02 -C 68000 -l /dev/stdout -o /tmp/blsload_sim.bin blsload_sim.asm -I ../include -i bls.inc -d BUS:=2 -d CHIP:=9 -d TARGET:=2

; Implemented calls :
; DRVINIT
; CDCSTAT
; CDCSTOP
; CDCACK
; ROMREAD
; ROMREADN
; ROMREADE
; CDCREAD
; CDCTRN

BDP_OUT_BUFSIZE equ     $30                     ; Address of data size in buffer
BDP_OUT_BUFFER  equ     $32                     ; Address of sub buffer
BDP_OUT_MAXSIZE equ     $2E                     ; Max number of bytes in buffer

SECBUF  set             $200                    ; Sector buffer
SECHEAD set             $A00                    ; Sector header in CDCTRN format

                org     $A04

BOOT            ; Simulated BIOS boot program

                ; Disable interrupts
                move    #$2700, sr
                SUB_INT_DISABLE

                ; Init stack pointer
                lea     $5E80, sp

                ; Clear comm flags
                clr.b   GA_COMMFLAGS_SUB
                clr.l   GA_COMMOUT
                clr.l   GA_COMMOUT + 4
                clr.l   GA_COMMOUT + 8
                clr.l   GA_COMMOUT + 12

                ; Initialize word ram to 2M mode, accessible from main cpu
                move.b  #$00, GA_MM + 1         ; Switch to 2M mode
.wait_2m
                move.b  #$02, GA_MM + 1         ; Give access to main cpu
                cmi.b   #$01, GA_MM + 1         ; Wait until switch is done
                bne.b   .wait_2m

                ; Clear all BIOS entry points
                lea     $5E80, a0
                move.w  #($6000 - $5E80) / 2 - 1, d0
.clear_entry    move.w  #$4E73, (a0)+           ; Write RTE instructions all over the place
                dbra    d0, .clear_entry

                ; Read SP entry table offset
                movea.l $6018.w, a0             ; Read entry table offset
                lea     $6000(a0), a0           ; Convert to RAM address 
                move.l  a0, d0                  ; Table is in a0 and d0

                ; Setup level 2 interrupt
                moveq   #0, d1
                move.w  4(a0), d1
                add.l   d0, d1
                ; d1 contains address to level 2 interrupt
                move.w  #$4EF9, $5F7C           ; JMP xxx.L
                move.l  d1, $5F7E               ; Target address

                ; Compute address to SP_MAIN
                moveq   #0, d1
                move.w  2(a0), d1
                add.l   d0, d1
                ; d1 contains address to SP_MAIN
                move.l  d1, CALL_SP_MAIN + 2    ; Patch JSR xxx.L instruction

                ; Compute address to SP_INIT
                moveq   #0, d1
                move.w  (a0), d1
                add.l   d0, d1
                move.l  d1, a1
                ; a1 contains address to SP_INIT
                jsr     (a1)                    ; Call SP_INIT

                ; Enable interrupts
                SUB_INT_ENABLE
                andi    #$F8FF, sr

                ; Loop SP_MAIN
call_sp_main
                jsr     RETURN_NOW.l            ; Target address patched by setup code
                bra.b   call_sp_main

BLSLOAD_SIM_ENTRY
                cmp.b   #$8A, d0
                beq.b   CDCSTAT

                cmp.b   #$89, d0
                beq.b   CDCSTOP

                cmp.b   #$8D, d0
                beq.b   CDCACK

                cmp.b   #$17, d0
                beq.b   ROMREAD

                cmp.b   #$20, d0
                beq.b   ROMREAD

                cmp.b   #$21, d0
                beq.b   ROMREAD

                cmp.b   #$8B, d0
                beq.b   CDCREAD

                cmp.b   #$8C, d0
                beq.b   CDCTRN

RETURN_NOW
                rts

CDCSTAT
                or.w    d0, d0                  ; Clear carry
                rts

CDCSTOP
CDCACK
                clr.l   SECHEAD.w
                rts

ROMREAD
                movem.l a0/a1, -(sp)
                lea             5(a0), a0
                lea             BDP_OUT_BUFFER.w, a1
                move.b  (a0)+, (a1)+
                move.b  (a0)+, (a1)+
                move.b  (a0)+, (a1)+
                jsr             BDP_WRITE_BUF(PC)
                movem.l (sp)+, a0/a1
                rts

; Sends 3 bytes in BDP_WRITE_BUF using BDP
BDP_WRITE_BUF
                move.w  #3, BDP_OUT_BUFSIZE.w
                bset    #5, GA_COMMFLAGS_SUB.w
.wait_flush
                tst.w   BDP_OUT_BUFSIZE
                bne.b   .wait_flush
                bclr    #5, GA_COMMFLAGS_SUB.w
                rts

CDCREAD
                movem.l a0/a1, -(sp)
                move.l  #$0F0F0F0F, BDP_OUT_BUFFER.w
                jsr             BDP_WRITE_BUF(PC)
                movem.l (sp)+, a0/a1
.wait_sector
                move.l  SECHEAD.w, d0           ; Test header, move it to d0 and clear carry
                beq.b   .wait_sector
                rts

CDCTRN
                movem.l d0/a1, -(sp)
                lea             SECBUF.w, a1
                move.w  #$1FF, d0
.trn_copy
                move.l  (a1)+, (a0)+
                dbra    d0, .trn_copy
                movem.l (sp)+, d0/a1
                move.l  SECHEAD.w, (a1)+
                rts


; vim: ts=8 sw=8 sts=8 et
