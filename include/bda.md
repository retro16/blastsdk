The Blast Debugger Agent : 

Allows to communicate with BDB through an arduino plugged in the gamepad port.

To use BDA, include this file in your binary, include "bda.inc" or "bda.h"
depending on your language, then call bda_init

Do not include this file if you use bls_init : bls_init automatically includes
bda as the standard procedure.


Example in asmx :

            include "bda.inc"
            
    MAIN
            bda_init


Example in C :

    #include "bda.h"

    void main()
    {
      bda_init();
    }


---------------------------------------

 - provides bda.asm bda_ram.asm

