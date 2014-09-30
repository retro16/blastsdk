                include bda.inc
                include beh.inc

IP_MAIN
MAIN
                VDPSECURITY                     ; Unlock TMSS
                beh_init                        ; Initialize exception handler
                bda_init                        ; Initialize debug agent
                jsr     monitor_init            ; Initialize monitor
                move.w  #$2000, sr              ; Enable interrupts
.1              bra.b   .1                      ; Monitor logic is in the vblank interrupt

; vim: ts=8 sw=8 sts=8 et
