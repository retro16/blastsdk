;;;;;
; blsvdp_dma(void *dest, void *src, unsigned int len);
; Send data to VRAM using DMA
blsvdp_dma_regs
                ; Commands to set VDP registers
                dw      $9300   ; R19 : DMA length LSB
                dw      $9400   ; R20 : DMA length MSB
                dw      $9500   ; R21 : DMA source MSB
                dw      $9600   ; R22 : DMA source middle
                dw      $9700   ; R23 : DMA source LSB
blsvdp_dma
                movem.l d2, -(sp)

                if CHIP != CHIP_CART
                lea     blsvdp_dma_regs(pc), a0    ; Point a0 at register settings
                else
                ; dma_regs in read-only memory : copy them before the stack
                movem.l blsvdp_dma_regs, d0/d1/d2
                lea     -12(sp), a0
                movem.l d0/d1/d2, (a0)
                endif

                lea     VDPCTRL, a1     ; Point a1 at VDP control port
               
                ; Read parameters
                movem.l 4(sp), d0-d2    ; d0 = dest, d1 = src, d2 = len

                if TARGET == TARGET_SCD1 || TARGET == TARGET_SCD2
                cmpi.b  #$20, 9(sp)     ; Source from word ram
                bne.b   .not_word_ram_1
                addq.l  #2, d1          ; Adjust address
.not_word_ram_1
                endif

                ; Prepare DMA source address
                lsr.l   #1, d1          ; Right-shift source address
                movep.l d1, 3(a0)       ; Set DMA source address (first byte will be overwritten)

                ; Prepare DMA length
                movep.w d2, 1(a0)
                
                ; Set VDP registers
                move.l  (aO)+, (a1)
                move.l  (aO)+, (a1)
                move.w  (aO)+, (a1)

                move.w  d0, d2          ; d2 = VRAM address MSB + DMA flag
                bset    #14, d0
                bclr    #15, d0
                move.w  d0, (a1)        ; Set VRAM LSB

                rol.w   #2, d2
                andi.w  #$0003, d2
                bset    #7, d2
                move.w  d2, -(sp)       ; Move to stack to work around hardware DMA glitch
                move.w  (sp)+, (a1)     ; Set VRAM MSB and start DMA

                if TARGET == TARGET_SCD1 || TARGET == TARGET_SCD2

                ; Second part of the workaround for Word RAM DMA
                cmpi.b  #$20, 9(sp)     ; Source from word ram
                bne.b   .not_word_ram_2

                ; Set VDP write
                move.w  d0, (a1)        ; Send VRAM LSB
                bclr    #7, d2          ; Disable DMA
                move.w  d2, (a1)        ; Set VRAM MSB

                ; Fetch first word from RAM
                lsl.l   #1, d1
                move.l  d1, a0
                move.w  (a0), VDPDATA   ; Write first word to VRAM

.not_word_ram_2
                endif

                movem.l (sp)+, d2       ; Restore altered d2 register
                rts

;;;;;
; Initialized prepared VDP buffer
; void blsvdp_prepare_init(short *prepared);
blsvdp_prepare_init
                move.l  4(sp), a0
                movei.w #$FFFF, (a0)
                rts

;;;;;
; Prepares VDP DMA transfer command in a VDP buffer.
; void blsvdp_prepare_dma(void *dest, void *src, unsigned int len, short *prepared);
blsvdp_prepare_dma
                move.l  16(sp), a0      ; Load target pointer to a1
                move.w  (a0), d0
                lea     0(a0, d0), a1
                addi.w  #7, (a0)

                ; Compute DMA length
                movei.l #$93009400, d0  ; Set VDP command
                move.b  14(sp), d0      ; Put MSB of len in VDP register 20
                swap    d0
                move.b  15(sp), d0      ; Put LSB of len in VDP register 19

                ; Send DMA length to VDP
                move.l  d0, (a1)+

                ; Compute DMA source address and copy mode
                lea     8(sp), a0
                roxr.w  (a0)            ; Right-shift source address
                roxr.w  10(sp)          ;
                move.b  #$97, (a0)      ; Register 23 : source address MSB

                ; Send source MSB to VDP
                move.w  (a0), (a1)+

                ori.l   #$95009600, d0  ; Prepare VDP registers 21 and 22
                move.b  11(sp), d0      ; Register 22 : source address middle byte
                swap    d0
                move.b  10(sp), d0      ; Register 21 : source address LSB

                ; Send DMA source address to VDP
                move.l  d0, (a1)+
                
                ; Compute DMA dest address
                lea     6(sp), a0
                move.w  (a0), d0
                andi.w  #$3FFF, d0
                ori.w   #$4000, d0

                swap    d0              ; Compute second DMA command
                move.w  (a0), d0
                rol.w   #2, d0
                andi.w  #$0003, d0
                ori.w   #$0080, d0

                ; Send DMA dest address (second command)
                move.l  d0, (a1)+

                ; Return pointer after data
                move.l  a1, d0

                rts

;;;;;
; Sends a prepared buffer to the VDP control port.
; void blsvdp_exec(short *prepared);
blsvdp_exec
                move.l  4(sp), a0
                if _VDPREG
                move    a5, a1
                else
                lea     VDPCTRL, a1
                endif

                move.w  (a0)+, d0       ; Read word count
                cmpi.w  #$FFFF, d0      ; FFFF = empty queue
                beq.b   vdp_send_finished
vdp_send
                move.w  (a0)+, (a1)     ; Send data to VDP control port
                dbra    d0, vdp_send

vdp_send_finished

                rts

; vim: ts=8 sw=8 sts=8 et
