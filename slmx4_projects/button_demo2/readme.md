# Button Demo 2

This program is demonstrates a timed button press

Hardware required:
- SLMX4-HW
- epam-x4 (or other pin-compatible x4 module)

This program demonstrates another way to GPIO interrupts. In situations where
a number of actions may be executed based on how long a button has been pressed, 
the typical *Timer* is not adequate. This example uses the `xTaskGetTickCount()`
function to measure elapsed time around a button press, then takes an appropriate 
action.

When the program runs, the RGB LED will be one of three colors:
- Red   (user button 2 pressed < 2 s)
- Green (user button 2 pressed > 2 s and < 4 s)
- Blue  (user button 2 pressed > 4 s)

User button 2 is also used as a separate task. This shows how the single ISR
can serve multiple FreeRTOS tasks in an independent fashion.

User button 1 is used to toggle a "throb" which manipulates the brightness
of the RGB LED color. User button 1 presses < 2 s are ignored, and > 2 s will
toggle the throbbing feature.