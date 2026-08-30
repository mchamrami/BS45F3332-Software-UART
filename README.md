I needed a simple serial debug output on a small Holtek MCU, without relying on a hardware UART peripheral.

So I ended up implementing a software UART transmitter on the BS45F3332 using GPIO bit-banging.

One detail I wanted to get right was the bit timing. Instead of checking each data bit with an if/else while the frame is being transmitted, I calculate all eight output states first. The actual UART frame is then just a fixed sequence of GPIO writes and delays.

Current setup:

Holtek BS45F3332
1 MHz system clock
PA2 / IPCK as TX
9600 baud
8N1
TX only
Interrupts disabled only during each frame

PA2 is shared with the programming clock pin, which also made this an interesting little exercise in reusing limited MCU resources.

I cleaned the UART part up as a standalone example and put the source code and implementation notes on GitHub.

[GitHub link]

#EmbeddedSystems #Firmware #EmbeddedC #Microcontrollers #UART #Holtek
