High level structures
=====================

Section
-------

Low level unit, represents one memory-mapped binary stream on the target system.
Sections can be groupped together in a group.


Source
------

A source is a group of sections linked to a source file.

For example, a ".c" file generates 4 sections : .text; .rodata, .data and .bss ; these sections are linked to the same ".c" source.


Binary
------

A binary is a group of sections loaded at the same time.

You call BLS loading functions on binaries, thus loading every section of the given group.

In the future, binary loading will be parallelized and optimized better.


Output
------

An output is a target for a given architecture. It represents the final ".bin" or ".iso" file. An output is a group of sections that will be put into the image.

There can be multiple outputs for a given project. Outputs can share sections as long as compiling sections do not require different parameters.



Process to generate an output
=============================

Create high level structures
----------------------------

 * Parse conf file
  - parse sources
	- parse sections
	- parse binaries
	- parse outputs

 * Find entry point

 * Generate binaries from outputs

 * Generate sources from binaries

 * Generate sections from sources

 * Finalize all sources

 * Finalize all sections
  - section.pfmt, section.name, section.symbol.name, section.symbol.value.chip

 * Finalize all binaries

 * Determine entry points

Create a dependency graph and a build order list (BOL)
------------------------------------------------------

For each source, try to compile without context and fetch external symbols

Determine the source used by entry point. Use it as the starting point for the following algorithm :

    If the source is in the BOL, return
    Add the source to the BOL
    For each section in source
        For each external symbol
            Find section defining this symbol
            Find source associated with this section
            When found, recurse with this source

Compute binary sizes
--------------------

    For each source in the BOL
        Generate the symbol table (use random values in the correct ranges for unknown external symbols)
        Use a predefined ORG in the correct address range (depending on chip/bus)
        Compile
        Get the resulting binary size
        Compress
        Get the compressed size ; each module should add a small margin if relocation can change compressed size

Map sections
------------

Now that all binary sizes, chips and busses are known, the final memory map can be computed :

    Sections that do not have a predefined address should be placed at their final position
        Use the first-match algorithm from blsbuild to do this

After mapping target addresses, physical addresses should be defined.

Symbols must be updated to reflect changes.

Generate final image
--------------------

Generate the header for the current medium.

All addresses are known. Proceed with the final compilation.
Binaries are generated into "section.datafile" and compressed directly to the correct offset into the final image.

Finally, for cart images, checksum is computed and put at the right offset.

