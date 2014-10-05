                include bls_init.inc
                include bls_vdp.inc
                include bda.inc

MAIN
IP_MAIN
                bls_init 0, INT_VBLANK
                bls_enable_interrupts
                VDPSETBG $080
                ;ENTER_MONITOR
                SYNC_MAIN_SUB
                bls_init_vdp 0, 1, 64, 64, $C000, 0, $E000, $B000, $BE00, 0, 0, 0, 0, 0, 0, 0
.1              bra.b .1


INT_VBLANK      SUB_INTERRUPT
                rte

; vim: ts=8 sw=8 sts=8 et
