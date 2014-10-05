; Provides SCD sub CPU initializatoin routines.
; You must include bls_init_sub.md in the same binary.

bls_init_sub    macro
                ccall   bls_init_sub
                endm

bls_enable_interrupts macro
                andi    #$F8FF, SR
                endm

bls_disable_interrupts macro
                ori     #$0700, SR
                endm

; vim: ts=8 sw=8 sts=8 et
