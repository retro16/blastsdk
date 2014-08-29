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
