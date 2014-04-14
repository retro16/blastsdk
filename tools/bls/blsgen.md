Binaries
========

Binary splash.c
---------------

 - bus main

Outputs
=======

An output defines an image of a physical medium. Many outputs can be generated from one project with different binaries and entry points, for example to generate a Genesis and a Sega-CD version of the same game.


Output mygame.img
-----------------

 - target gen (or scd or vcart, deduced from output name if ending in img, gen, iso or vcart)
 - region JUE
 - copyright 2014 GPL
 - author Retro 16
 - binaries splash.c
 
 - entry splash
