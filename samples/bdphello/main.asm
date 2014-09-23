MAIN
    VDPSECURITY
.1
    CCALL2  bdp_write, textdata, textend-textdata
    ENTER_MONITOR
    bra.b .1

textdata
    DB  "Hello world\n"
textend
