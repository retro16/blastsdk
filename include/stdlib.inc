; void *memcpy(void *dest, const void *src, size_t n);
memcpy
        move.l  4(sp), a0           ; dest
        move.l  8(sp), a1           ; src
        move.l  12(sp), d1          ; count
        beq.b   .memcopy_end        ; If count is zero, just return

        ; Check if optimization is worthwhile
        moveq   #8, d0
        cmp.l   d0, d1              ; Not enough bytes to copy to use heavy opimization
        blo.b   .bytecopy           ; Copy byte per byte

        ; Check if src and dest have the same alignment
        move.l  a0, d0
        add.l   a1, d0              ; Add the 2 values to combine their last bit
        btst    #0, d0              ; Last bit is clear if the 2 addresses have the same alignment
        bne.b   .bytecopy           ; Jump if source and destination have different alignments

        ; Align to word boundary
        move.l  a0, d0
        btst    #0, d0
        beq.b   .alignedcopy        ; Already aligned
        move.b  (a1)+, (a0)+        ; Copy the leading unaligned byte
        subq.l  #1, d1
.alignedcopy

        moveq   #$40, d0            ; Test if copy is really large
        cmp.l   d0, d1
        blo.b   .longcopy           ; Not large enough for a big block copy

        ; Big block copy (block size : 32 bytes, constant time : 212, loop time : 170)
        move.l  d1, d0
        lsr.l   #6, d0
        subq.l  #1, d0
        movem.l d2-d7/a2-a3, -(a7)  ; Store intermediate registers

.bigblock
        ; Copy 32 bytes at a time
        movem.l (a1)+, d2-d7/a2-a3
        movem.l d2-d7/a2-a3, (a0)
        lea     32(a0), a0
        dbra    d0, .bigblock
        
        movem.l (a7)+, d2-d7/a2-a3  ; Restore intermediate registers

        ; Compute remaining bytes
        moveq   #31, d0
        and.l   d0, d1
        beq.b   .memcopy_end        ; Jump if no more data to copy

        ; Long word copy (block size : 4 bytes, constant time : 52, loop time : 34)
.longcopy
        move.l  d1, d0
        lsr.l   #2, d0
        subq.l  #1, d0

.longcopy_loop
        move.l  (a1)+, (a0)+        ; Fast copy with double words
        dbra    d0, .longcopy_loop

        ; Compute remaining bytes
        moveq   #3, d0
        and.l   d0, d1
        beq.b   .memcopy_end        ; Jump if no more data to copy

        ; Byte per byte copy (constant time : 8, loop time : 22)
.bytecopy
        subq.l  #1, d1

.bytecopy_loop
        move.b  (a1)+, (a0)+        ; Byte per byte copy
        dbra    d1, .bytecopy_loop

.memcopy_end
        ; Return
        move.l  4(sp), d0
        rts


; void *memset(void *s, int c, size_t n);
memset
        move.l  12(sp), d1          ; count
        beq.b   .memset_end         ; If count is zero, just return
        move.l  4(sp), a0           ; dest
        move.w  10(sp), d0          ; byte
        bne.b   .bytefill           ; If byte is not zero, special code is not used

        cmp.l   #$60, d1            ; Skip optimizations for small sizes
        blo.b   .bytefill

        movem.l d2-d7/a2-a3, -(a7)  ; Store for block copy

        move.l  a0, d2
        btst    #0, d2
        bne.b   .aligned
        move.b  d0, (a0)+           ; Write the first, unaligned byte
        subq.l  #1, d1
.aligned

        moveq   #0, d2
        moveq   #0, d3
        moveq   #0, d4
        moveq   #0, d5
        moveq   #0, d6
        moveq   #0, d7
        clr.l   a2
        clr.l   a3

        move.l  d1, d0              ; d0 stores the number of dbra loops
        lsr.l   #5, d0              ; divide by 32 (32 bytes per loop)
        subq.l  #1, d0
.blockclear_loop
        movem.l d2-d7/a2-a3, (a0)+
        dbra    d0, .blockclear_loop

        movem.l (a7)+, d2-d7/a2-a3  ; Restore data registers

        moveq   #31, d0             ; Compute remaining bytes to fill
        and.l   d0, d1

.bytefill
        subq.l  #1, d1
.bytefill_loop
        move.b  d0, (a0)+
        dbra    d1, .bytefill_loop

.memset_end
        ; Return
        move.l  4(sp), d0
        rts


; vim: ts=8 sw=8 sts=8 et
