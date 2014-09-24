; Macros to call C functions

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

ENTER_MONITOR   macro
                trap    #07
                endm

; vim: ts=8 sw=8 sts=8 et
