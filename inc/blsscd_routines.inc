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


; vim: ts=8 sw=8 sts=8 et
