"""
Example how to operate the SLM-X4 Health firmware using a manual
polling technique.

Note:
By using manual polling, there is likely going to be much more
jitter than by using streaming which locks the radar framerate
to a fixed amount.

Copyright (C) 2022 Sensor Logic
Written by Justin Hadella
"""
import os
import time

from slmx4_health_wrapper import slmx4_health
from slmx4_health_debug import *

# Create a instance of the slmx4 health wrapper
slmx4 = slmx4_health('/dev/ttyACM0') # Linux
# slmx4 = slmx4_health('COM3') # Windows

# Open the USB VCOM connection
slmx4.open()

# Get the version info and display
ver = slmx4.get_version()
print('ver =', ver)

# The version info is a tuple which indicates:
# (firmware name, firmwave version, protocol version)

print('Press CTRL-C to terminate loop')

# Delay 1 second to give user time to see version info and prompt
time.sleep(1)

# Run loop until the user presses CTRL-C
try:
	while True:
		# Trigger a single radar measurement
		health, resp_wave = slmx4.one_shot()

		if os.name == 'nt':
			os.system('cls')
		else:
			os.system('clear')

		debug_health(health)
		debug_resp_wave(resp_wave)
		
		# Delay 100 ms to simulate 10 FPS data rate
		time.sleep(0.1)

except KeyboardInterrupt:
    pass

# Close the USB VCOM connection
slmx4.close()