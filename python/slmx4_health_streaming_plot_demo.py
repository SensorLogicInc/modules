"""
Example how to operate the SLM-X4 Health firmware using a data
streaming technique.

Note:
The radar data rate is fixed in hardware at 10 frames/second.

Note:
This version also plots the respiration waveform in a matplotlib
window. The interval argument is made small so the true rate is
based on the rate that the health & respiration data arrive.

Copyright (C) 2022 Sensor Logic
Written by Justin Hadella
"""
import os
import matplotlib.pyplot as plt

from matplotlib.animation import FuncAnimation

from slmx4_health_wrapper import slmx4_health
from slmx4_health_debug import *

def animate(i):
	# When data streaming, the SLM-X4 will send two messages back-to-back:
	# the health message, followed by the respiration waveform
	health = slmx4.read_msg()
	resp_wave = slmx4.read_msg()

	if os.name == 'nt':
		os.system('cls')
	else:
		os.system('clear')
	
	# Display the health info on the terminal
	debug_health(health)

	# Get the frame counter value
	fc = health.health.debug[0]

	# Add the frame counter to the plot title
	title = 'Respiration Waveform (fc =' + str(fc) + ')'

	x = range(resp_wave.vector.len)
	y = resp_wave.vector.vec

	ax.clear()
	ax.plot(x, y)

	ax.set_xlim([0, resp_wave.vector.len])
	ax.set_ylim([-2, 2])
	ax.set_title(title)

# Create the plot window
fig, ax = plt.subplots()

# Create a instance of the slmx4 health wrapper
slmx4 = slmx4_health('/dev/ttyACM0') # Linux
# slmx4 = slmx4_health('COM3') # Windows

# Open the USB VCOM connection
slmx4.open()

# Start data streaming at fixed rate
slmx4.start()

# The data streaming/plotting is in animate()
ani = FuncAnimation(fig, animate, interval=10)
plt.show()

# Stop data streaming
slmx4.stop()

# Close the USB VCOM connection
slmx4.close()