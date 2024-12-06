# pbitx
![Pbitx](/docs/media/panorama_small.JPG)

Pbitx is a replacement for the arduino in Ubitx v6, it´s based on the Raspberry Pi Pico micro controller.

It contains software written in C, the code is a rework of the ubitxv6-master branch some files are replaced/added.
The name is a combination of Pico and Ubitx resulting in Pbitx.

Changed or replaced files is for instance the file handling CW, it's replaced with my own state machine. I also use my own CI-V interpreter.
Beside thiose files I have made a lot of changes in the code, and I like to think that I have simplified some code.
I use a set of fonts from http://www.rinkydinkelectronics.com/r_fonts.php/ thank you for those files.

I've added a S-meter and a panorama-mode, both dependent on a extended ND6T AGC ciricuit. The panaroma mode means that a broader frequency
span is sweept and presented as s spectrum on the display, this is how it was done in ancient times, of course the reciever isn't usable
in this mode. Pbitx has three CW modes, straight key, plain respective iambic bugg mode. CAT is now a subset of the CI-V interrface. 
For now it's just a couple of commands implemented such as setting and reading the frequency, changing mode and switch between RX and TX mode.
The structure of the interpreter makes it very easy to add more of the CI-V commands.

Hardware design files can be found in the PCB folder and the subfolder AGC.
There are two Kicad designs the first on is the adapter that replace the Arduino with a RPI Pico controller. The second one is an
AGC detector it is used by some functions, S-Meter and a panoram scope, The detector is based on ND6T's design with some extra components added.

This is, of course, a work in progress and features will be added and bugg fixed.

Bengt - SM0KBW 	2023-Nov-01 






