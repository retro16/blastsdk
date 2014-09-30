Blast Debugger Protocol

Send data to BDB from the genesis using

    void BDP_WRITE(const char *data, int length)

Can be used on sub CPU, but you have to call this function often on main
CPU (for example at each VBLANK) :

    void BDP_SUB_CHECK()


---------------------------------------

 - provides bdp.asm

