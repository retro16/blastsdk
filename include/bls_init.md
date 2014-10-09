Include this and "bls_init.h" or "bls_init.inc", then call bls_init to
initialize the current CPU automatically.

The header also provides a VDP initialization routine.

Example (code on main CPU):

    #include <bls_init.h>

    BLS_INT_CALLBACK on_vblank() {
      // vblank code
    }

    void main() {
      bls_init(NULL, on_vblank);
      // Your main loop ...
    }


 - provides bls_init.asm bda.asm bda_ram.asm beh.asm

