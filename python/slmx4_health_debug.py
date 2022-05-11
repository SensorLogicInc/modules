"""
Contains a collection of helper functions to print the contents of messages
from the SLM-X4 Health Firmware

Copyright (C) 2022 Sensor Logic
Written by Justin Hadella
"""
import slmx4_usb_vcom_pb2 as pb

def debug_health(msg):
	# Function prints out the Health message data if appropriate
	if msg.opcode == pb.HEALTH_MSG:
		health = msg.health
		print('presence_detected =', health.presence_detected)
		print('respiration_detected =', health.respiration_detected)
		print('movement_detected =', health.movement_detected)
		print('movement_type =', health.movement_type)
		print('distance =', health.distance)
		print('distance_conf =', health.distance_conf)
		print('respiration_rpm =', health.respiration_rpm)
		print('respiration_conf =', health.respiration_conf)
		print('rms =', health.rms)
		print('temperature =', health.temperature)
		print('humidity =', health.humidity)
		print('lux =', health.lux)
		print('frame count =', health.debug[0])
		print('minutes =', health.debug[1])

def debug_resp_wave(msg):
	# Function prints out the Respiration Waveform data if appropriate
	if msg.opcode == pb.ONE_SHOT:
		resp_wave = msg.vector
		print('len =', resp_wave.len)
		print('vec = [', end='')
		for i in range(resp_wave.len):
			print(resp_wave.vec[i], end=' ')
		print(']')