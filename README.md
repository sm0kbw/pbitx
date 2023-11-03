# pbitx
![Pbitx](/docs/media/panorama_small.JPG)

Pbitx is a replacement for the arduino in Ubitx v6, it´s based on the Raspberry Pi Pico micro controller.

It contains software written in C, the code is a rework of the ubitxv6-master branch some files are replaced/added.
The name is a combination of Pico and Ubitx resulting in Pbitx.

Changed or replaced files is for instance the file handling CW here I use my own state machine instead. I use my own CI-V interpreter
instead of the original. Beside thiose files I have made a lot of changes in the code, and I like to think that I have simplified some
code. I use a set of fonts from http://www.rinkydinkelectronics.com/r_fonts.php/ thank you for thoswe files.

Hardware design files can be found in the PCB folder and the subfolder AGC.
There are two Kicad designs the first on is the adapter that replace the Arduino with a RPI Pico controller. The second one is an
AGC detector it is used by some functions, S-Meter and a pan-scope, The detector is based on ND6T's design with some extra components added.

Bengt - SM0KBW 	2023-Nov-01 






