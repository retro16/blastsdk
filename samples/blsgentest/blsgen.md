This program is a test program for blsgen.
Displays a scrollable hello world on screen.

It generates both Genesis, SCD and vcart programs.


Source test.c
======
 - type gcc
 - section .text
  - addr $300


Output **Genesis**
==================

Compiled binaries:
 - binary main
  - source main.asm

Output properties:
 - name **BLSGEN Test**
 - target gen
 - region JUE
 - file blsgentest.bin
 - entry main


Output **VCart**
================

Compiled binaries:
 - binary main
  - source main.asm

Output properties:
 - name **BLSGEN Test**
 - target vcart
 - region JUE
 - file blsgentest_vcart.bin
 - entry main

