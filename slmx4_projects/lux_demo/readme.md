# Lux Demo

This program is simple demonstration of the LTR308 Ambient Light Sensor

Hardware required:
- SLMX4-HW
- epam-x4 (or other pin-compatible x4 module)

When the program runs, the lux reading is displayed and the RGB LED is lit
green with its brightness controlled by the lux value.

The ambient lux value should be relatively constant normally. When you block
the light (with you hand or finger) the lux value will drop. When you shine a
light at the sensor, the lux value should increase.