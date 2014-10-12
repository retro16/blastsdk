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

                org     $000200                 ; BIOS boot entry point

BOOT            ; Simulated BIOS boot program

                ; Disable interrupts
                SUB_INT_DISABLE

                ; Clear comm flags
                clr.b   GA_COMMFLAGS_SUB
                clr.l   GA_COMMOUT
                clr.l   GA_COMMOUT + 4
                clr.l   GA_COMMOUT + 8
                clr.l   GA_COMMOUT + 12

                ; Initialize word ram to 2M mode, accessible from main cpu
                move.b  #$01, GA_MM + 1         ; Switch to 2M mode
.wait_2m
                move.b  #$01, GA_MM + 1         ; Give access to main cpu
                cmpi.b  #$01, GA_MM + 1         ; Wait until switch is done
                bne.b   .wait_2m

                ; Clear all BIOS entry points
                lea     $6000, sp
                move.w  #($6000 - $5E80) / 2 - 1, d0
.clear_entry    move.w  #$4E73, -(sp)           ; Write RTE instructions all over the place
                dbra    d0, .clear_entry

                ; Stack pointer is now initialized at $005E80

                ; Set BIOS call entry point
                move.w  #$4EF9, $5F22.w         ; JMP xxx.L
                move.l  #BLSLOAD_SIM_ENTRY, $5F24.w

                ; Read SP entry table offset
                movea.l $6018.w, a0             ; Read entry table offset
                lea     $6000(a0), a0           ; Convert to RAM address 
                move.l  a0, d0                  ; Table is in a0 and d0

                ; Setup level 2 interrupt
                moveq   #0, d1
                move.w  4(a0), d1
                beq.b   .no_l2
                add.l   d0, d1
                ; d1 contains address to level 2 interrupt
                move.w  #$4EF9, $5F7C           ; JMP xxx.L
                move.l  #L2_HANDLER, $5F7E      ; Target address
                move.l  d1, L2_JUMP + 2         ; Patch handler with target address
.no_l2

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

L2_HANDLER
                movem.l d0-d7/a0-a6, -(sp)
L2_JUMP         jsr     RETURN_NOW.l
                movem.l (sp)+, d0-d7/a0-a6
                rte


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
                bls.b   ROMREAD

                cmp.b   #$8B, d0
                beq.b   CDCREAD

                cmp.b   #$8C, d0
                beq.b   CDCTRN

RETURN_NOW
                rts

CDCSTAT
                or.b    d0, d0                  ; Clear carry
                rts

CDCSTOP
CDCACK
                clr.l   SECHEAD.w               ; Reset header
                rts

ROMREAD
                movem.l d0/a0/a1, -(sp)
                addq    #1, a0
                lea     BDP_OUT_BUFFER.w, a1
                move.b  (a0)+, (a1)+
                move.b  (a0)+, (a1)+
                move.b  (a0)+, (a1)+
                jsr     BDP_WRITE_BUF(PC)
                movem.l (sp)+, d0/a0/a1
                rts

; Sends 3 bytes in BDP_OUT_BUFFER using BDP
BDP_WRITE_BUF
                move    sr, d0
                ori     #$0700, sr              ; Disable interrupts while waiting
                
                move.w  #3, BDP_OUT_BUFSIZE.w
                bset    #5, GA_COMMFLAGS_SUB.w

.wait_flush
                tst.w   BDP_OUT_BUFSIZE
                bne.b   .wait_flush

                bclr    #5, GA_COMMFLAGS_SUB.w

                move    d0, sr
                rts

CDCREAD
                movem.l a0/a1, -(sp)
                move.l  #($07000000 | (SECBUF << 8)), BDP_OUT_BUFFER.w
                jsr     BDP_WRITE_BUF(PC)
                movem.l (sp)+, a0/a1
.wait_sector
                move.l  SECHEAD.w, d0           ; Test header, move it to d0 and clear carry
                beq.b   .wait_sector
                rts

CDCTRN
                movem.l d0/a1, -(sp)
                lea     SECBUF.w, a1

                ; Test if dest address is unaligned
                move.w  a0, d0
                btst    #0, d0
                bne.b   .unaligned

                ; Aligned destination: use long word copy
                move.w  #$1FF, d0
.trn_copy_long
                move.l  (a1)+, (a0)+
                dbra    d0, .trn_copy_long
                bra.b   .finish

                ; Unaligned destination: use slower byte copy
.unaligned
                move.w  #$7FF, d0
.trn_copy_byte
                move.l  (a1)+, (a0)+
                dbra    d0, .trn_copy_byte

                ; Restore damaged registers except a0 and write header
.finish
                movem.l (sp)+, d0/a1
                move.l  SECHEAD.w, (a1)+        ; Write header and clear carry to return success
                rts

                ; Data buffer follows
SECBUF                                          ; Sector buffer starts here
SECHEAD         set     SECBUF + $800           ; Sector header in CDCTRN format

                assert  SECHEAD < $5000         ; Ensure there is enough room for the user stack.

; vim: ts=8 sw=8 sts=8 et
