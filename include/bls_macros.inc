; This file contains asm-specific macros not available in the C API.

; Macros to call C functions

CCALL   macro   routine
                jsr     routine
                endm

CCALL1  macro   routine, p1
                pea     p1
                jsr     routine
                addq.l  #4, a7
                endm
                
CCALL2  macro   routine, p1, p2
                pea     p2
                pea     p1
                jsr     routine
                addq.l  #8, a7
                endm
                
CCALL3  macro   routine, p1, p2, p3
                pea     p3
                pea     p2
                pea     p1
                jsr     routine
                lea     12(a7), a7
                endm
                
CCALL4  macro   routine, p1, p2, p3, p4
                pea     p4
                pea     p3
                pea     p2
                pea     p1
                jsr     routine
                lea     16(a7), a7
                endm
                
CCALL5  macro   routine, p1, p2, p3, p4, p5
                pea     p5
                pea     p4
                pea     p3
                pea     p2
                pea     p1
                jsr     routine
                lea     20(a7), a7
                endm
                
CCALL6  macro   routine, p1, p2, p3, p4, p5, p6
                pea     p6
                pea     p5
                pea     p4
                pea     p3
                pea     p2
                pea     p1
                jsr     routine
                lea     24(a7), a7
                endm
                
CCALL7  macro   routine, p1, p2, p3, p4, p5, p6, p7
                pea     p7
                pea     p6
                pea     p5
                pea     p4
                pea     p3
                pea     p2
                pea     p1
                jsr     routine
                lea     28(a7), a7
                endm
                
CCALL8  macro   routine, p1, p2, p3, p4, p5, p6, p7, p8
                pea     p8
                pea     p7
                pea     p6
                pea     p5
                pea     p4
                pea     p3
                pea     p2
                pea     p1
                jsr     routine
                lea     32(a7), a7
                endm

LOOPWHILE       macro   instruction, p2
.WHILE_LOOP\?   instruction ,p2
                bne.b   .WHILE_LOOP\?
                endm

LOOPUNTIL       macro   instruction, p2
.UNTIL_LOOP\?   instruction ,p2
                beq.b   .UNTIL_LOOP\?
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

; vim: ts=8 sw=8 sts=8 et
