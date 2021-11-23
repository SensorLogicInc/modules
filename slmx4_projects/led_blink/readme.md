# LED Blink Demo

This program is simple demonstration of the LEDs

Hardware required:
- SLMX4-HW
- epam-x4 (or other pin-compatible x4 module)

This program uses the Red, Green, and RGB LED in a few modes. Under the "Modes"
section below, uncomment one mode and comment out all the others. _Some modes
can be mixed though._

Modes:
* BLINK_RED_LED  
  This mode blinks the Red LED at a fixed rate
* BLINK_GREEN_LED  
  This mode blinks the Green LED at a fixed rate
* BLINK_RGB_LED  
  This blinks the RGB LED at a fixed rate. This essentially is setting the RGB
  LED to either the current color or "off"
* COLOR_CYCLE_RGB_LED  
  This mode cycles the RGB LED through the HSV color space by manipulating the
  _hue_ value
* THROB_RGB_LED  
  This mode color cycles the RGB LED by manipulating the _hue_ as in the color
  cycle mode. However, this mode also manipulates the _value_ in the HSV color
  space, which changes the brightness
* NIGHT_THROB_LED  
  This mode throbs the brightness for night time use in a heartbeat pattern
