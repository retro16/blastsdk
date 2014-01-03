Format
======

 - key value (comment after first word)
  - subkey subvalue
  - subkey list of values separated by token

Keys are always lower case starting by a letter and alphanumeric only.
Comments are in parentheses.
To escape values, enclose in **double asterisks** or __double underscores__ (bold).
Titles act as keys with empty value.

Transformed to the following struct:

    struct node {
     char key[64];
     char *value;
     struct node *next;
     struct node *child;
     struct node *parent;
    };

List of values are transformed to siblings with the same key name. Children of the list node are copied to every node. For example :

 * listexample a b c
  * subnode

equals :

 * listexample a
  * subnode
 * listexample b
  * subnode
 * listexample c
  * subnode

Sources
=======

source main
-----------
 - file main.c
 - type gcc
 - chip rom

source blsloader.asm
--------------------

source splash
-------------
 - file splash.png
 - type png
 - chip vram

source level1
-------------
 - file tilemap.bin
 - type bin
 - chip vram
 - align $1000

source maincharacter
--------------------
 - file mainchar.png
 - type pngspr
 - width 32
 - chip vram


Binaries
=========

Binaries are elements loaded by loading routines. For cartridge, the loader initializes data or uploads to VRAM. For CD, all sources in the resource are copied from the physical medium to their target addresses.

binary initial
--------------
 - source main blsloader.asm splash

binary level1
-------------
 - avoid blsloader.asm (do not overwrite blsloader.asm)
 - source levelcode.c (uses default values)
 - source maincharacter
 - source level1

Output
======

 - target genesis (or cd or vcart)
 - region JUE
 - name testbin (binary output)
 - entry main (symbol name)

