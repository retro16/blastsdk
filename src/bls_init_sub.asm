; Initializes the Genesis in a useable state.
; Synchronizes with the Sega CD

; Sub CPU :
;    void bls_init_sub();
bls_init_sub
                ; Initialize Sega CD part

                ; Enable interrupt mask
                move.w  #GA_IM_L1 | GA_IM_L2 | GA_IM_L3 | GA_IM_L4 | GA_IM_L5 | GA_IM_L6, GA_IMASK
                ;move.w  #GA_IM_L2, GA_IMASK

                rts

; vim: ts=8 sw=8 sts=8 et
