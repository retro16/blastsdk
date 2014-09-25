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
;       andi.w  #$F8FF, $8004.w
;       ori.w   #$0300, $8004.w
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
        rts

.loadquery      ; Main CPU asked for loading
        move.l  BLSCDR_COMM, d0         ; Get sector number from main CPU
        andi.l  #$00FFFFFF, d0          ; Filter out sector count

        moveq   #0, d1                  ; Get sector count from main CPU
        move.b  BLSCDR_COMM, d1         ;

        movem.l d0/d1, BLSLOAD_SECTOR   ; Store data in ROMREADN format
        lea     BLSLOAD_SECTOR, a0      ; Execute ROMREADN
        BIOS_ROMREADN

        if ..DEF BLSLOAD_DMA
        move.b  #GA_CDC_DEST_WRAMDMA, BLSLOAD_DEST      ; Set destination to word RAM

.reset_interrupt_handler
        move.l  #.checkflag, BLSLOAD_INTERRUPT_HANDLER + 2
        rts
        
        else    ; !BLSLOAD_DMA
        ; Use simple blocking read
        if TARGET == TARGET_SCD1
        lea     $0C0000, a0
        else
        lea     $080000, a0
        endif
        move.l  d1, d0
        lsl.l   #11, d0
        jsr     BLSLOAD_READ_CD

        ; Send WRAM back to main cpu
        if TARGET == TARGET_SCD1
        SWAP_WRAM_1M
        else
        assert TARGET == TARGET_SCD2
        SEND_WRAM_2M
        endif
        rts
        endif   ; !BLSLOAD_DMA



; Stream d0 words from CD-ROM to address starting at a0
; Registers :
;  d0.l : number of words to copy
;  d1.l : number of words to read from current sector
;  d2.l : number of words in current transfer
;  a0.l : target address
; When done, a0 contains ending address
BLSLOAD_READ_CD
        movem.l d2, -(sp)
        move.w  BLSLOAD_BUFOFF, d1      ; d1 : remaining bytes in current sector
        bne.b   .cdctrn                 ; There is still some data in the read buffer

        ; Set CDC in sub cpu read mode
        move.b  #GA_CDC_DEST_SUBREAD, GA_CDC_DEST

.cdcstat
        BIOS_CDCSTAT                    ; Wait until data is in CD-ROM cache
        bcs.b   .cdcstat

.cdcread
        BIOS_CDCREAD                    ; Trigger a read from CD-ROM
        bcs.b   .cdcread
        move.l  #$400, d1               ; Set amount of data to be read in current sector

.cdctrn
        ; Compute if sector data or target size is the smallest
        cmp.l   d0, d1
        blo.b   .smallcopy

        ; Number of bytes to copy is greater than data remaining in the current sector
        move.l  d1, d2                  ; Use remaining bytes as iterator
        sub.l   d2, d0                  ; Update copy size already
        bra.b   .trn_one_sector

.smallcopy
        move.l  d0, d2                  ; Use target size as iterator
        sub.l   d2, d1                  ; Update remaining size already

.trn_one_sector
        ; Transfer data from CDC host data
        subq.w  #1, d2                  ; Adjust d2 for dbra
        loopuntil btst #GA_DSR, GA_CDC  ; Wait for data
.trn    move.w  GA_CDCHD, (a0)+
        dbra    d2, .trn

        BIOS_CDCACK                     ; Acknowledge sector

        tst.l   d0
        bne.b   .cdcstat                ; Read next sector
        
        move.w  d1, BLSLOAD_BUFOFF
        movem.l (sp)+, d2
        rts
        
; vim: ts=8 sw=8 sts=8 et
