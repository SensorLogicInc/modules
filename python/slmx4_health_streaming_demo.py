"""
Example how to operate the SLM-X4 Health firmware using a data
streaming technique.

Note:
The radar data rate is fixed in hardware at 10 frames/second.

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

# Start data streaming at fixed rate
slmx4.start()

# Run loop until the user presses CTRL-C
try:
	while True:
		# When data streaming, the SLM-X4 will send two messages back-to-back:
		# the health message, followed by the respiration waveform
		health = slmx4.read_msg()
		resp_wave = slmx4.read_msg()

		if os.name == 'nt':
			os.system('cls')
		else:
			os.system('clear')

		debug_health(health)
		# debug_resp_wave(resp_wave)

except KeyboardInterrupt:
    pass

# Stop data streaming
slmx4.stop()

# Close the USB VCOM connection
slmx4.close()