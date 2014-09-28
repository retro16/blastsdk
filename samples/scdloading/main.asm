        include bda.inc
        include beh.inc
        include bls_init.inc

IP_MAIN
        bls_init 0, VBLANK
        bls_init_vdp 0, 1, 64, 64, $C000, 0, $E000, $B000, $BE00, 0, 0, 0, 0, 0, 0, 0

VBLANK
        SUB_INTERRUPT
        rte

; vim: ts=8 sw=8 sts=8 et
