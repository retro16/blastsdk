        if ..UNDEF BLS_NBDA
        if TARGET != TARGET_GEN
                org     $FFFDBE
                ds      $1F8                    ; Final code place
        else
                org     $FFFFB6                 ; On the genesis, code stays in ROM
        endif

bda_pkt_header  ds      4                       ; Space reserved for communication

; CPU state when not interrupted

bda_cpu_state
bda_regs                                        ; Space reserved for registers
bda_d0          ds      4
bda_d1          ds      4
bda_d2          ds      4
bda_d3          ds      4
bda_d4          ds      4
bda_d5          ds      4
bda_d6          ds      4
bda_d7          ds      4
bda_a0          ds      4
bda_a1          ds      4
bda_a2          ds      4
bda_a3          ds      4
bda_a4          ds      4
bda_a5          ds      4
bda_a6          ds      4
bda_sp
bda_a7          ds      4
bda_pc          ds      4                       ; PC register value
bda_sr          ds      2                       ; SR register value
bda_cpu_state_end
; End of CPU state

                ; Ensure that this code is placed at the end of main RAM
                assert  (bda_cpu_state_end & $FFFFFF) == 0
        ENDIF

; vim: ts=8 sw=8 sts=8 et
