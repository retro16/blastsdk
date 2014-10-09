Blast Debugger Protocol

Send data to BDB from the genesis using

    void bdp_write(const char *data, int length);

Can be used on sub CPU, but you have to call this function often on main
CPU (for example at each VBLANK) :

    void bdp_sub_check();


---------------------------------------

 - provides bdp.asm

