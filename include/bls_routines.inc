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

; Repeat instruction with zero or one parameter count times
BLSREPEAT macro count, instruction
    if (count) > 256
    BLSREPEAT 256, instruction
    BLSREPEAT ((count)-256), instruction
    elsif (count) > 64
    BLSREPEAT 64, instruction
    BLSREPEAT ((count)-64), instruction
    elsif (count) > 16
    BLSREPEAT 16, instruction
    BLSREPEAT ((count)-16), instruction
    elsif (count) > 4
    BLSREPEAT 4, instruction
    BLSREPEAT ((count)-4), instruction
    elsif (count) > 0
    instruction 
    BLSREPEAT ((count)-1), instruction
    endif
    endm

; Repeat instruction with 2 parameters count times
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
; dest and src can be anything supported by LEA.
; dest can also be a1 and src can be a0.
; Addresses must be aligned to a word boundary.
; bytes is the number of bytes to copy. It must be <= 2097152.
; returns address past dest in a1 and past src in a0
BLSFASTCOPY_ALIGNED macro dest, src, bytes
    if bytes == 0
    else

    if src != a0
    lea     src, a0                     ; a0 = src : Load source and dest address
    endif
    if dest != a1
    lea     dest, a1                    ; a1 = dst
    endif

    if bytes >= $60

    movem.l d2-d7/a2, -(a7)
    ; Compute d0 = block count, a0 = src address, a1 = dest address
    if bytes < ($100*$20)               ; Try to load value into d0 using moveq
    moveq   #(bytes/$20)-1, d0          ; Compute number of $20 bytes blocks
    else
    move.w  #(bytes/$20)-1, d0          ; Use slower move immediate
    endif

    ; Do block copy
.blockcopy_loop\?
    movem.l (a0)+, d1-d7/a2
    movem.l d1-d7/a2, (a1)
    lea     $20(a1), a1
    dbra    d0, .blockcopy_loop\?

    movem.l (a7)+, d2-d7/a2

    ; Copy long data remainder with move.l expansion
    BLSREPEAT2 (bytes & $1F) / 4, move.l (a0)+, (a1)+

    else
    // Expanded move.l copy
    BLSREPEAT2 bytes / 4, move.l (a0)+, (a1)+
    endif

    BLSREPEAT2 (bytes & 3) / 2, move.w (a0)+, (a1)+
    BLSREPEAT2 (bytes & 1), move.b (a0)+, (a1)+

    endif
    endm

BLS_SETINTVECTOR macro vect, target
    if SCD != 0
    movem.l d0/a0, -(a7)
    ; Put target in d0 and vector in a0
    move.l  #target, d0
    lea     vect, a0
    cmp.l   d0, (a0)        ; Check if vector in ROM already jumps to the correct address
    beq.b   .vector_set\?
    move.l  (a0), a0        ; Point a0 to the target of the interrupt handler
    move.w  #$4EF9, (a0)+   ; Write JMP instruction
    move.l  d0, (a0)+       ; JMP to the target
.vector_set\?
    movem.l (a7)+, d0/a0
    endif
    endm

; vim: ts=8 sw=8 sts=8 et
