; Copy aligned words
BLSFASTCOPY_WORD macro dest, src, words
    if words == 0
    elsif words == 1
    move.w  src, dest
    elsif words == 2
    move.l  src, dest
    else
    movem.l d0/a0/a1, -(a7)
    lea     src, a0
    lea     dest, a1
    if words <= 128
    clr     d0
    moveq   words - 1, d0
    else
    move.w  words - 1, d0
    endif
.fastcopy\?
    move.w  (a0)+, (a1)+
    dbra    d0, .fastcopy\?
    movem.l (a7)+, d0/a0/a1
    endif
    endm


; Fill memory with the same word value
BLSFASTFILL_WORD macro dest, words, value
    if words == 0
    elsif words == 1
    move.w  dest, value
    else
    movem.l d0/a0, -(a7)
    lea     dest, a1
    if words <= 128
    clr     d0
    moveq   words - 1, d0
    else
    move.w  words - 1, d0
    endif
.fastfill\?
    move.w  value, (a0)+
    dbra    d0, .fastfill\?
    movem.l (a7)+, d0/a0
    endif
    endm


BLSREPEAT2 macro count, instruction, p2
    if (count) > 256
    BLSREPEAT2 256, instruction, p2
    BLSREPEAT2 ((count)-256), instruction, p2
    elsif (count) > 64
    BLSREPEAT2 64, instruction, p2
    BLSREPEAT2 ((count)-64), instruction, p2
    elsif (count) > 16
    BLSREPEAT2 16, instruction, p2
    BLSREPEAT2 ((count)-16), instruction, p2
    elsif (count) > 4
    BLSREPEAT2 4, instruction, p2
    BLSREPEAT2 ((count)-4), instruction, p2
    elsif (count) > 0
    instruction , p2
    BLSREPEAT2 ((count)-1), instruction, p2
    endif
    endm


; Copy memory from word-aligned addresses.
; dest and src can be anything supported by LEA, except something based on a1. Addresses must be aligned to a word boundary.
; bytes is the number of bytes to copy. It must be <= 2097152.
BLSFASTCOPY_ALIGNED macro dest, src, bytes
    if bytes == 0
    else

    if bytes >= $60

    movem.l d0-d7/a0-a2, -(a7)
    ; Compute d0 = block count, a1 = src address, a2 = dest address
    lea     src, a1                     ; a1 = src : Load source and dest address
    lea     dest, a2                    ; a2 = dst
    if bytes < ($100*$20)               ; Try to load value into d0 using moveq
    moveq   #(bytes/$20)-1, d0          ; Compute number of $20 bytes blocks
    else
    move.w  #(bytes/$20)-1, d0          ; Use slower move immediate
    endif

    ; Do block copy
.blockcopy_loop\?
    movem.l (a1)+, d1-d7/a0
    movem.l d1-d7/a0, (a2)
    lea     $20(a2), a2
    dbra    d0, .blockcopy_loop\?

    movem.l (a7)+, d0-d7/a0
    ; Copy long data remainder
    BLSREPEAT2 (bytes & $1F) / 4, move.l (a1)+, (a2)+
    BLSREPEAT2 (bytes & 3) / 2, move.w (a1)+, (a2)+
    BLSREPEAT2 (bytes & 1), move.b (a1)+, (a2)+
    else
    movem.l a1-a2, -(a7)
    lea     src, a1
    lea     dest, a2
    BLSREPEAT2 bytes / 4, move.l (a1)+, (a2)+
    BLSREPEAT2 (bytes & 3) / 4, move.w (a1)+, (a2)+
    BLSREPEAT2 (bytes & 1), move.b (a1)+, (a2)+
    endif

    movem.l (a7)+, a1-a2
    endif
    endm

