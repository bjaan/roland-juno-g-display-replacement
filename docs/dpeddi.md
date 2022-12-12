
Roland Juno G LCD Emulator
==========================

The keyboard Roland Juno G is equipped with an "International Display Works" V0054850 Rev A LCD with PN: 5485SGPABNC

It seems that this part is defective and Roland replaced it with a new revision that was quite expansive
and nowadays isnt available any more.

I looked for a different display unit that would fit the old one.
I've choosen this one: https://www.aliexpress.com/item/1005003033844928.html 

It is jus a little smaller then the old one but it should fit well, but the interface is SPI, while the 
original display was parallel.

Perfect to reduce the number of pins required to connect to a microcontroller.

I connected to the keyboard my hantek 6022BL Logic probe and tried to sniff the data.
The original Hantek software was really poor, so i moved to Sigrok.
After few attempts I had my csv with the sniffed data.
I wrote a small python program to parse the sniffed data and visual look to a result..
Finally i understood the protocol.

I can assume that the original display have two KS0713 controllers that control half display each other.

I connected the keyboard and the display to my ESP32 (I started dreaming to change the color using wifi), 
and after some tests i discovered that the speed of the keyboard bus was too fast to use IRQ, so i tried 
to move to DMA by using the CS1 line as reference clock for the I2C controller.
It worked.. i get half display.. but by reading the ESP32 datasheet I discovered that the second I2C controller
can't be used for a secondary parallel input..

After few reading I decided to buy a Raspberry Pi Pico. The PIO seems really promising...
I connected it... the display connected to SPI is 2 or 3 time faster then ESP... great...
So i implemented a sketch to read data using IRQ... but even the RP2040 failed... 
so i moved to State Machine + DMA... it was a pain... few documentation id didn't worked at all..


I've studied some of the following project located on github to learn something more about dma  
https://github.com/raspberrypi/pico-examples/blob/master/pio/logic_analyser/logic_analyser.c  
https://github.com/gamblor21/rp2040-logic-analyzer/blob/main/rp2040-logic-analyzer.c  
https://github.com/panda5mt/picampinos  
https://github.com/ArduCAM/arducam_mic/blob/master/arducam_mic.c  
https://github.com/pinballpower/code_dmd  
https://github.com/jostlowe/Pico-DMX  

After some attempts i choosed with Pico-DMX, adapted to my needs to get the display working.


BOM:
====

- Raspberry Pi Pico
- Display ILI9488 5.0"
- Some jumper wires
- Some solder and some dil jumper and a solder board and thin cables.

Pinout:
=======

![alt text](https://github.com/dpeddi/LCDJunoG/blob/main/image.png?raw=true)

Conntecting the RPI pico to the display and the keyboard is quite easy.
You just need to find out the keyboard display pinout by reading the service manual and invent something to connect the rpi to the keyboard.

|JunoG Pin|JunoG Function|RpiPico Pin|
|---|---|---|
|18|+5V|VSYS|
|17|GND|GND|
|16|+3V||
|15|RST|GP14|
|14|CS1|GP12|
|13|CS2|GP13|
|12|RS|GP11|
|11|WE|GP10|
|10|D0|GP2|
|9|D1|GP3|
|8|D2|GP4|
|7|D3|GP5|
|6|D4|GP6|
|5|D5|GP7|
|4|D6|GP8|
|3|D7|GP9|
|2|BRGT|GP26|
|1|BRGT Vref|+3V|
