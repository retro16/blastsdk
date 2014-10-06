        if TARGET == TARGET_SCD1 || TARGET == TARGET_SCD2

                include cdbios.inc
        
; Read d1 sectors starting at d0 into buffer in a0 (using Sub-CPU)
ReadCD
                movem.l d0-d1/a0-a1, -(sp)
.0
                move.w  #$0089, d0      ;CDCSTOP
                jsr     $5F22.w         ;call CDBIOS function

                movea.l sp, a0          ;ptr to 32 bit sector start and 32 bit sector count
                move.w  #$0020, d0      ;ROMREADN
                jsr     $5F22.w         ;call CDBIOS function
.1
                move.w  #$008A, d0      ;CDCSTAT
                jsr     $5F22.w         ;call CDBIOS function
                bcs.b   .1              ;no sectors in CD buffer

                ;set CDC Mode destination device to Sub-CPU
                andi.w  #$F8FF, $FFFF8004.w
                ori.w   #$0300, $FFFF8004.w
.2
                move.w  #$008B, d0      ;CDCREAD
                jsr     $5F22.w         ;call CDBIOS function
                bcs.b   .2              ;not ready to xfer data

                movea.l 8(sp), a0       ;data buffer
                lea     12(sp), a1      ;header address
                move.w  #$008C, d0      ;CDCTRN
                jsr     $5F22.w         ;call CDBIOS function
                bcs.b   .0              ;failed, retry

                move.w  #$008D, d0      ;CDCACK
                jsr     $5F22.w         ;call CDBIOS function

                addq.l  #1, (sp)        ;next sector
                addi.l  #2048, 8(sp)    ;inc buffer ptr
                subq.l  #1, 4(sp)       ;dec sector count
                bne.b   .1

                lea     16(sp), sp      ;cleanup stack
                rts



; Level 2 interrupt handler for blsgen binary loader.
; JMP or JSR here at every level 2 interrupt
BLSLOAD_INTERRUPT_HANDLER
                jmp     .checkflag      ; Jmp target altered by the code
.checkflag
                btst    #BLSCDR_BIT, GA_COMMFLAGS_MAIN
                bne.b   .loadquery
.disable
                rts

.loadquery      ; Main CPU asked for loading
                move.l  BLSCDR_COMM, d0         ; Get sector number from main CPU
                andi.l  #$00FFFFFF, d0          ; Filter out sector count
        
                moveq   #0, d1                  ; Get sector count from main CPU
                move.b  BLSCDR_COMM, d1         ;

                BIOS_CDCSTOP
                lea     BLSLOAD_SECTOR, a0
                movem.l d0/d1, (a0)             ; Store data in ROMREADN format
                BIOS_ROMREADN

        if ..DEF BLSLOAD_DMA
                move.b  #GA_CDC_DEST_WRAMDMA, BLSLOAD_DEST      ; Set destination to word RAM

.reset_interrupt_handler
                move.l  #.checkflag, BLSLOAD_INTERRUPT_HANDLER + 2
                rts
        
        else    ; !BLSLOAD_DMA

                ; Restore level 2 interrupts and disable BLSLOAD interrupt processing
                move.l  #.disable, BLSLOAD_INTERRUPT_HANDLER + 2
                move.w  #$2000, sr

        ; Use simple blocking read
        if TARGET == TARGET_SCD1
                lea     $0C0000, a0
        else
                lea     $080000, a0
        endif
                move.l  d1, d0
;FIXME                lsl.l   #8, d0                  ; TODO : optimize slow shift
;                lsl.l   #3, d0
;                jsr     BLSLOAD_READ_CD_ASM

        ; Send WRAM back to main cpu
        if TARGET == TARGET_SCD1
                SWAP_WRAM_1M
        else
                assert TARGET == TARGET_SCD2
                SEND_WRAM_2M
        endif

        ; Restore BLSLOAD interrupt handling
                move.l  #.checkflag, BLSLOAD_INTERRUPT_HANDLER + 2
                rts
        endif   ; !BLSLOAD_DMA


; void BLSLOAD_START_READ(u32 sector, u32 count);
BLSLOAD_START_READ
                BIOS_CDCSTOP
                move.w  #$800, BLSLOAD_BUFOFF
                lea     BLSLOAD_COUNT, a0
                move.l  8(sp), (a0)
                move.l  4(sp), -(a0)
                BIOS_ROMREADN
                rts


; void BLSLOAD_READ_CD(u32 bytes, u32 target);
BLSLOAD_READ_CD

; Stream <bytes> from CD-ROM to <target> address
; Registers :
;  a3.l : target address (must be word aligned)
;  d5.l : number of bytes to copy (must be even)
;  d4.w : sector size in bytes
;  d3.l : offset in intermediate data buffer
;  a2.l : transfert address for retry
; Returns :
;  When done, a2 contains ending address
                movem.l a2/a3/d3/d4/d5, -(sp)
                movem.l 24(sp), d5/a3           ; Read C parameters to registers
                move.w  #$800, d4               ; Constant

                move.w  BLSLOAD_BUFOFF, d3      ; Update value from RAM
                cmp.w   d4, d3
                blo.w   .flush                  ; There is still some data in the read buffer

.cdcstat
                BIOS_CDCSTAT                    ; Wait until data is in CD-ROM cache
                bcs.b   .cdcstat

                ; Set CDC in sub cpu read mode
                move.b  #GA_CDC_DEST_SUBREAD, GA_CDC_DEST

.cdcread
                BIOS_CDCREAD                    ; Trigger a read from CD-ROM
                bcs.b   .cdcread

                cmp.w   d4, d5
                blo.b   .indirect               ; Copy will leave data in the buffer

                ; Direct transfert
                lea     BLSLOAD_HEADER(pc), a1
                move.l  a3, a2
                move.l  a3, a0
                BIOS_CDCTRN                     ; a0 will be updated by the routine
                bcc.b   .success_direct
                jsr     .retry
.success_direct
                move.l  a0, a3                  ; Update target address
                addi.l  #1, -12(a1)             ; Update BLSLOAD_SECTOR
                subi.l  #1, -8(a1)              ; Update BLSLOAD_COUNT

                move.w  d4, d3                  ; Buffer is considered empty after a full transfert
                sub.w   d4, d5                  ; Update remaining data
                bra.b   .ack

                ; Indirect transfert (read from buffer)
.indirect
                lea     BLSLOAD_DATA(pc), a0
                lea     -4(a0), a1
                move.l  a0, a2
                BIOS_CDCTRN                     ; Send data to the intermediate buffer
                bcc.b   .success_indirect
                jsr     .retry
.success_indirect
                addi.l  #1, -12(a1)             ; Update BLSLOAD_SECTOR
                subi.l  #1, -8(a1)              ; Update BLSLOAD_COUNT
                moveq   #0, d3                  ; Reset buffer index

.flush          ; Copy data from BLSLOAD_DATA buffer

                move.w  d4, d0
                sub.w   d3, d0                  ; d0 = $800-d3 : remaining bytes in the buffer
                cmp.w   d5, d0
                bls.b   .large                  ; d0 <= d5 : the whole buffer will be copied
                move.w  d5, d0                  ; too much data in the buffer : copy only what the caller requested
.large
                
                ; d0 == number of bytes to be copied in the current buffer

                lea     BLSLOAD_DATA(pc), a1    ; a1 <- offset in data
                lea     (a1, d3.w), a1

                ; Update byte count registers
                add.w   d0, d3                  ; Move the cursor
                sub.w   d0, d5                  ; Update copy amount
  movem.l d0-d7/a0-a7, $7F00         ; DEBUG
                lsr.w   #2, d0                  ; Convert to long count
                bcc.b   .long_aligned           ; Second bit clear : even number of words
                move.w  (a1)+, (a3)+            ; Copy first word
                tst.w   d0                      ; Restore CC on d0
.long_aligned
                beq.b   .ack                    ; Transfer was only 2 bytes !
                subq    #1, d0                  ; Adjust for dbra

  movem.l d0-d7/a0-a7, $7F80         ; DEBUG
.copy_loop
                move.l  (a1)+, (a3)+
                dbra    d0, .copy_loop

.ack
                BIOS_CDCACK                     ; Acknowledge sector

                tst.l   d5
                bne.w   .cdcstat                ; Read next sector
        
                move.w  d3, BLSLOAD_BUFOFF
                movem.l (sp)+, a2/a3/d3/d4/d5
                rts


                ; Retry routine
.track
                dc.b    1                       ; Read TOC from track #1
                dc.b    255                     ; Read all tracks

.tries          dc.w    5                       ; Retries before reinit

.reinit         ; Reinitialize drive if needed and retry read
                subi.w  #1, .tries
                bne.b   .retry

                ; 5 retries : reinitialize
                lea     .track(pc), a0
                BIOS_DRVINIT
                move.w  #5, .tries              ; Reset retry counter

                ; Retry a failed read (d2 = dest address)
.retry
                BIOS_CDCSTOP

                lea     BLSLOAD_SECTOR, a0
                BIOS_ROMREADN

.retry_stat     BIOS_CDCSTAT
                bcs.b   .retry_stat

.retry_read     BIOS_CDCREAD
                bcs.b   .retry_read

                move.l  a2, a0
                lea     BLSLOAD_HEADER, a1
                BIOS_CDCTRN
                bcs.b   .reinit                 ; Failed again : reinitialize the drive and retry

                move.w  #5, .tries              ; Reset retry counter

                rts

        endif

; vim: ts=8 sw=8 sts=8 et
