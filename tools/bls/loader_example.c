/* Genesis example */

MACRO BLSLOAD_LEVEL1()
do {
  blsload_copy(LEVEL1_DATA_PHYSADDR, LEVEL1_DATA, LEVEL1_DATA_SIZE); // Main RAM upload
  blsload_copy(LEVEL1_MUSIC_PHYSADDR, LEVEL1_MUSIC, LEVEL1_MUSIC_SIZE); // Z80 upload
  blsload_dma(LEVEL1_GFX_PHYSADDR, LEVEL1_GFX, LEVEL1_GFX_SIZE); // VRAM upload
} while(0)

int myprogram()
{
  BLSLOAD_LEVEL1();
}


/* Sega CD example
 * Code must be run on sub CPU
 */

MACRO BLSLOAD_LEVEL1()
do {
  blsload_queue(LEVEL1_DATA_SECT, MRAM, LEVEL1_DATA, LEVEL1_DATA_SIZE); // Main RAM upload
  blsload_queue_offset(LEVEL1_MUSIC_SECT, LEVEL1_MUSIC_SOFF, ZRAM, LEVEL1_MUSIC, LEVEL1_MUSIC_SIZE); // Z80 upload
  blsload_queue(LEVEL1_GFX_SECT, WRAM, LEVEL1_GFX, LEVEL1_GFX_SIZE); // Word RAM upload
} while(0)


/* Sega CD load routine */

All loading operations are initiated by the sub CPU. Provided macros are not compatible with main CPU.
Communication use one commword (Sub=>Main).
When loading to PRAM, PCMRAM or WRAM, the Sub CPU uses DMA and does not need the commword nor main CPU.

When idle, CW (commword) is set to $FFFF.

To initiate a Sub=>Main transfer, sub sets CW to a WRAM address (expressed in multiple of 4 bytes). When switching WRAM, main CPU reads the CW address and searches for a small descriptive structure in WRAM :

1 byte : Number of copies (0 = 1 entry, 1 = 2 entries, ...)
3 bytes : source address
4 bytes : target address ($FFFF0000-$FFFFFFFF for VRAM DMA upload)
2 bytes : size
For each remaining copy :
4 bytes : source address
4 bytes : target address ($FFFF0000-$FFFFFFFF for VRAM DMA upload)
2 bytes : size

Adresses are direct pointers in main CPU bus, except for VRAM which uses DMA.


Sega CD background task stack
=============================

 * Stack frame size
 * Handler pointer
 * Handler parameters
 * Stack frame size
 * Handler pointer
 * Handler parameters
End of stack

Because the structure is arranged like a stack, sequential operations must be pushed backwards. This is important for CD loading operations.

A handler must pop its data once done, leaving the stack exactly on the next handler ID.

The blsyield() function interrupts the current background task and resumes it on next level 2 interrupt.

Return value of background functions are ignored.


Function call macro example
---------------------------

Example for this function :

    void my_bg_fct(int iterations, int incr) {
      volatile int *a = (volatile int *)0x34111;
      int i;
      for(i = 0; i < iterations; i += incr) {
       *a = i;
       blsyield();
      }
    }

To call this function, use the BLS_BG_CALL(fct, params) macro :

BLS_BG_CALL(my_bg_fct, (12, 3));

This macro expands to the following code :

    // Globals
    volatile void *sp1;
    volatile void *sp2;
    int *bg_stack_head;

    // Macro expansion
    BLS_SP_STORE(sp1);
    blsbg_pushparameters params;
    *(void**)(&bg_stack_head[1]) = (void*)(&fct);

    // Functions
    void blsbg_pushparameters(...) {
      BLS_SP_STORE(sp2);
      int framesize = (char*)sp2 - (char*)sp1;
      (char*)bg_stack_head -= framesize;
      memcpy(bg_stack_head, sp2, framesize); // Move parameter values to the BG task stack
      --bg_stack_head;
      *bg_stack_head = framesize;
    }


Background task algorithm
-------------------------

On each level 2 interrupt :
 * If a background task SP was set, read it to SP register and RTS
 * Point to the top of task stack
 * While the stack is not empty :
  * Pop one word and find the function entry point in a jump table
  * Call the function
  * If the function blsyield() is called, background task is interrupted
 * End while
 
blsyield() interrupt routine :
 * Store SP to RAM, restore normal SP and RTS


CD loading background task
--------------------------

Handler parameters :






68000 Function call stack
-------------------------

**With frame pointer, MRTD unset**

 * FP
 * SP
 * Param2
 * Param1
 * Param0

**Without frame pointer, MRTD unset**

 * SP
 * Param2
 * Param1
 * Param0


