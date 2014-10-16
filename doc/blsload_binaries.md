The binary loading system in blsload / blsgen
=============================================

The Blast SDK (more precisely blsgen and blsload) provide a notion of binary.

On modern operating systems, a binary (or "executable file") is often a file in PE or ELF format, with headers telling how to load the executable, where to load it in RAM and how to relocate it. Since console games have a finite number of executable files (i.e. ne for the menu, one for main gameplay, one for a bonus stage with a different gameplay, ...), you can hardcode all the information instead of putting it in a header. Takng the logic further, the location of the binary on the physical medium (cart / CD) can be hardcoded too, and the mechanism used to load all its parts can be hardcoded in the load call.

This hardcoding can be tedious to do by hand, especially when the project is evolving at a fast pace. So blsgen provides mechanisms to load binaries, and it goes further by allowing multiple binaries to reside in memory at the same time, so you can access globals of a binary from another, given both of them have been loaded.

Example: a RPG game
-------------------

Let's examine the memory map of a RPG, with levels containing different enemies and a player with a few stats (level, strength, inventory ...)

In this example, there are 2 levels: the first one has 3 types of enemies and a boss. The second one has more data, but only 2 types of enemies, sharing "Small enemy 2" with level 1.

Here is a diagram showing memory mapping: the column on the left is mapping when the first level is loaded, and the column on the right shows mapping when the second binary is loaded.

     --------------------------------- 
    | Binary 1       | Binary 2       |
    |---------------------------------|
    | Player stats (level, inventory) |
    |---------------------------------|
    |                |                |
    | Level 1 data   | Level 2 data   |
    |                |                |
    |                |                |
    |                |                |
    |                |                |
    |                |                |
    |----------------|                |
    |                |                |
    | Small enemy 1  |                |
    | code / stats   |----------------|
    |                | Small enemy 4  |
    |---------------------------------|
    |                                 |
    | Small enemy 2                   |
    | code / stats                    |
    |                                 |
    |---------------------------------|
    |                |                |
    | Small enemy 3  | (empty)        |
    | code / stats   |                |
    |                |----------------|
    |                |                |
    |----------------|                |
    |                | Level 2 boss   |
    | Level 1 boss   |                |
    |                |                |
    |                |                |
     --------------------------------- 

The game is split in 2 binaries:

 - The first binary contains level data, but also the player and small enemy 2 data. In blsgen terms, it *provides* all the sections needed to play level 1.

 - The second binary will only provide level 2 data, small enemy 4 and level 2 boss. Since "Player stats" and "Small enemy 2" are also present in this level, the variables defined in the first binary will be referenced by the second binary (for example when showing the player inventory or in the rendering loop, to render enemy 2). In blsgen terms, the second binary *uses* sections "Player stats" and "Small enemy 2".

Since binary 2 does not use, say, Level 1 boss, it can re-allocate other data at the same place, but it cannot allocate over Small enemy 2.


Loading and dependencies
------------------------

When loading a binary, all its provided sections are guaranteed to be available at their respective memory locations. Used sections won't be touched and must contain already valid data.

In other words, the programmer must ensure the proper load order of binaries, taking dependencies in account manually. In the example above, you must load the level 1 binary before being able to load the level 2 binary, or else data for used sections (player stats and small enemy 2) won't be loaded.

To load a binary, use the following instruction in your code (example loading level1.c):

ASMX:

        BLSLOAD_BINARY_LEVEL1_C

C:

    BLSLOAD_BINARY_LEVEL1_C();

The suffix `LEVEL1_C` must be replaced by the name of your binary, in capital letters with all special characters replaced by an underscore. When loading binaries on main CPU of the sega CD, the sub CPU will not be completely stalled, but some CPU time will be spent in interrupts to copy data to word RAM ; also, the whole binary will be copied in the WRAM, overwriting its content.

On the Sega CD, binaries can only contain sections targetting the same bus. Also, binaries targetting the main CPU can only loaded by the main CPU and binaries targetting the sub CPU can only be loaded by the sub CPU.


Allocation and loading strategies
---------------------------------

Depending on which medium the game is done, blsgen will use different allocation policies.

On cartridge games, sections in *cart* chip will be used in-place. If a section is provided by more than one binary, it will not be duplicated, thus producing binaries with non-contiguous data.

On CD-ROM games, where seek time is the most important problem, sections provided by more than one binary will be duplicated on the physical medium. Beware though, that their content will be strictly identical, so the code cannot be relocated between many copies of the same section.

