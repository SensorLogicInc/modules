# Button Demo

Hardware required:
- SLMX4-HW
- epam-x4 (or other pin-compatible x4 module)

This is a very simple demonstration of a "one-shot" timer in FREERTOS

This program is based on the buttontime demo for the RT 1060 EVK board, but
this uses two buttons:
- User Button 1 -- Controls the Red LED
- User Button 2 -- Controls the Green LED

Initially, both LEDs will blink slow at 1 Hz rate. However, if a button is 
pressed briefly, that button's corresponding LED will then blink at a much 
higher rate. If the button is pressed for > 2 s, then it will blink at the 
slower rate again.