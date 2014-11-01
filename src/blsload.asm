        if TARGET == TARGET_SCD1 || TARGET == TARGET_SCD2

                include cdbios.inc


; Check if the main CPU made a load request
; You should call this at each level 2 interrupt
; void BLSLOAD_CHECK_MAIN();
BLSLOAD_CHECK_MAIN
                jmp     .idle.l                 ; Jmp target altered by the code
.idle      
                btst    #BLSCDR_BIT, GA_COMMFLAGS_MAIN
                bne.b   .loadquery

                rts

                nop
.loadquery      ; Main CPU asked for loading
        if TARGET == TARGET_SCD1
                move.l  #$0C0000, BLSLOAD_TARGET
                nop
                nop
                move.l  #.wait_wram_1m, BLSLOAD_CHECK_MAIN + 2
.wait_wram_1m
                btst    #GA_DMNA_BIT, GA_MM + 1
                bne.b   .wram_1m_ready
                rts
.wram_1m_ready
        else
                assert TARGET == TARGET_SCD2
                move.l  #$080000, BLSLOAD_TARGET

                ; Wait for 2M RAM to be available
                nop                             ; Delay a bit to avoid missing the flag
                nop                             ; by a few cycles !
                move.l  #.wait_wram_2m, BLSLOAD_CHECK_MAIN + 2
.wait_wram_2m
                btst    #GA_RET_BIT, GA_MM + 1
                beq.b   .wram_2m_ready
                rts
.wram_2m_ready
        endif

                move.l  BLSCDR_COMM, d0         ; Get sector number from main CPU
                andi.l  #$00FFFFFF, d0          ; Filter out sector count
        
                moveq   #0, d1                  ; Get sector count from main CPU
                move.b  BLSCDR_COMM, d1         ;

                movem.l d0/d1, BLSLOAD_SECTOR   ; Store data in ROMREADN format

.romread_retry
                BIOS_CDCSTOP
                lea     BLSLOAD_SECTOR, a0
                BIOS_ROMREADN

                ; Wait for the next sector on next interrupt
.cdcstat
                move.l  #.cdcstat, BLSLOAD_CHECK_MAIN + 2
                BIOS_CDCSTAT
                bcc.b   .sector_available
                rts                             ; No sector available, wait until next interrupt
.sector_available

                ; Set CDC in sub cpu read mode
                move.b  #GA_CDC_DEST_SUBREAD, GA_CDC_DEST

                move.l  #.cdcread, BLSLOAD_CHECK_MAIN + 2
.cdcread        BIOS_CDCREAD
                bcc.b   .cdcread_success
                rts
.cdcread_success

                lea     BLSLOAD_HEADER, a1
                move.l  -16(a1), a0             ; Load BLSLOAD_TARGET
                BIOS_CDCTRN
                bcc.b   .cdctrn_success

                subi.w  #1, BLSLOAD_TRIES
                bne.b   .romread_retry          ; Retry before resetting drive

                ; Retried 5 times: reset the drive before retrying
                lea     DRVINIT_PARAMS(pc), a0
                BIOS_DRVINIT
                move.w  #5, BLSLOAD_TRIES       ; Reset retry counter
                bra.b   .romread_retry

.cdctrn_success BIOS_CDCACK

                move.l  a0, BLSLOAD_TARGET      ; Save current address to RAM
                ; a1 points at BLSLOAD_DATA
                addi.l  #1, -12(a1)             ; Update BLSLOAD_SECTOR
                subi.l  #1, -8(a1)              ; Update BLSLOAD_COUNT

                bne.w   .cdcstat                ; Read next sector if BLSLOAD_COUNT is not zero

                ; All sectors have been read.

                ; Send WRAM back to main cpu
        if TARGET == TARGET_SCD1
                SWAP_WRAM_1M
        else
                assert TARGET == TARGET_SCD2
                SEND_WRAM_2M
        endif

                ; Wait until the main CPU acknowledges by clearing its CDR flag
                loopwhile btst #5, GA_COMMFLAGS_MAIN

                ; Restore BLSLOAD interrupt handling
                move.l  #.idle, BLSLOAD_CHECK_MAIN + 2

                rts


; void BLSLOAD_START_READ(u32 sector, u32 count);
BLSLOAD_START_READ
                BIOS_CDCSTOP
                lea     BLSLOAD_COUNT, a0
                move.w  #5, -8(a0)              ; Reset BLSLOAD_TRIES
                move.w  #$800, -6(a0)           ; Reset BLSLOAD_BUFOFF
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
                lsr.w   #2, d0                  ; Convert to long count
                bcc.b   .long_aligned           ; Second bit clear : even number of words
                move.w  (a1)+, (a3)+            ; Copy first word
                tst.w   d0                      ; Restore CC on d0
.long_aligned
                beq.b   .ack                    ; Transfer was only 2 bytes !
                subq    #1, d0                  ; Adjust for dbra

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

.reinit         ; Reinitialize drive if needed and retry read
                subi.w  #1, BLSLOAD_TRIES
                bne.b   .retry

                ; 5 retries : reinitialize
                lea     DRVINIT_PARAMS(pc), a0
                BIOS_DRVINIT
                move.w  #5, BLSLOAD_TRIES       ; Reset retry counter

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

                rts

DRVINIT_PARAMS
                dc.b    1                       ; Read TOC from track #1
                dc.b    255                     ; Read all tracks

        endif

; vim: ts=8 sw=8 sts=8 et
