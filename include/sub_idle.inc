        if ..UNDEF sub_idle_h
sub_idle_h set 1

sub_idle_init macro
        if TARGET == TARGET_SCD1 || TARGET == TARGET_SCD2
                sync_main_sub
        endif
        endm

sub_idle_vsync macro
        if TARGET == TARGET_SCD1 || TARGET == TARGET_SCD2
                sub_interrupt
                bdp_sub_check
        endif
        endm

        endif

; vim: ts=8 sw=8 sts=8 et
