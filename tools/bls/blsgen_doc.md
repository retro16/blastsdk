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

      *Parse conf file
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



      Find symbols in sources
      -----------------------

      A first compilation pass is done on each source, to find symbols provided by the source. This pass also determines binaries loaded by each section.

      Compute binary sizes
      --------------------

      For each source in the BOL
      Generate the symbol table (use random values in the correct ranges for unknown external symbols)
          Use a predefined ORG in the correct address range (depending on chip/bus)
          Compile
          Get the resulting binary size
          Compress

          Get the compressed size ; each module should add a small margin if relocation can change compressed size

Determine entry point
---------------------

Find the entry point section and/or symbol (depending on target).

  Known sections prepass
  ----------------------

  This pass finds all sections provided by each binary based on its 'provides_sources' list.

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




      Create binary dependencies
      --------------------------

  For each section not provided by a binary, find another section depending on this one : the two sections are provided by the same binary.
      For each binary, find 'loads'.
  For each binary, find uses_binaries : all binaries used directly. Remove 'loads' from 'uses_binaries'.

      Now all binaries know all their dependencies.

      For each section, find its binary and all 'uses_binaries' ; add all their provided sections to the 'uses' list of the section.

Map sections
------------

Now all sections have a view of their dependencies when loaded, the final memory map can be computed :

Before, premap sources : values that can be guessed (especially chip/bus) are filled, so each source and section has as much information as possible.


Sections that do not have a predefined address should be placed at their final position
  Use the first-match algorithm from blsbuild to do this

    After mapping target addresses, physical addresses should be defined.

    Symbols must be updated to reflect changes.

    Final compilation
    -----------------

    Each source is compiled with final ORG. A first compilation pass is done on every source to find symbol values.

    Then the last compilation is done with all external symbols known.

    Each section is then extracted to an individual file.

    Generate final image
    --------------------

    Generate the header for the current medium.

    Binaries are generated and compressed directly to the correct offset into the final image.

    Finally, for cart images, checksum is computed and put at the right offset.

