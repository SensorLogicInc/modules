"""
Wrapper for SLM-X4 Health firmware using the USB VCOM interface

Copyright (C) 2022 Sensor Logic
Written by Justin Hadella
"""
import queue
import serial
import struct

from threading import Thread

import slmx4_usb_vcom_pb2 as pb

class slmx4_health():

	def __init__(self, port):
		self._usb = serial.Serial()
		self._usb.port = port
		self._is_open = False
		self._msg_queue = queue.Queue()
		self._msg_thread = Thread(daemon=True, target=self._read_thread)

	def open(self):
		try:
			self._usb.open()
		except Exception as e:
			print('Could not connect to slm-x4!')
			raise e
		
		self._is_open = True
		self._usb.flushInput()
		self._msg_thread.start()

	def close(self):
		self._usb.close()
		self._is_open = False

	def get_version(self):
		# Request the VERSION
		self._send_command(pb.VERSION)
		
		# Get the ACK
		ack = self._read_ack(pb.VERSION)

		# Get the VERSION info
		rsp = self._read_msg(pb.VERSION)

		# VERSION info is is comma-delimited string
		return rsp.str.str.split(',')
	
	def one_shot(self):
		# Request a ONE_SHOT radar measurement
		self._send_command(pb.ONE_SHOT)

		# Get the ACK
		ack = self._read_ack(pb.ONE_SHOT)

		# Get the Health message
		health = self._read_msg(pb.HEALTH_MSG)

		# Get the Respiration wave
		resp_wave = self._read_msg(pb.ONE_SHOT)
		
		return health, resp_wave

	def start(self):
		# Start data streaming
		self._send_command(pb.START)

		# Get the ACK
		ack = self._read_ack(pb.START)
		return

	def stop(self):
		# Stop data streaming
		self._send_command(pb.STOP)

		# Get the ACK
		ack = self._read_ack(pb.STOP)
		return

	def read_msg(self):
		# Read the message
		while True:
			rsp = self.read_from_queue()
			if rsp is not None:
				break

		return rsp

	def _send_command(self, opcode):
		# Encode ONE_SHOT command
		client_cmd = pb.client_command_t()
		client_cmd.opcode = opcode
		cmd_as_str = client_cmd.SerializeToString()

		# Send the command w/ encoded length
		self._usb.write(struct.pack('I', len(cmd_as_str)) + cmd_as_str)
	
	def _read_thread(self):
		while True:
			msg = self._read_pb_msg()
			if msg is not None:
				self._msg_queue.put_nowait(msg)
	
	def read_from_queue(self, **kwargs):
		return self._msg_queue.get(*kwargs)
	
	def _read_pb_msg(self):
		'''
		Message format:
		[len][msg]

		[len] is encoded as fixed length integer indicating the [msg] length
		'''
		# Read the [len]
		msg = self._usb.read(4)
		
		# Extract the [len] value
		len = struct.unpack('I', msg)
		if type(len) == tuple:
			len = len[0]
		
		# Read the [msg]
		msg = self._usb.read(len)
		rsp = pb.server_response_t()
		rsp.ParseFromString(msg)

		return rsp
	
	def _read_msg(self, opcode):
		# Read the message
		while True:
			rsp = self.read_from_queue()
			if rsp.opcode == opcode:
				break

		return rsp

	def _read_ack(self, opcode):
		# Read the [ACK]
		rsp = self._read_msg(pb.ACK)

		if rsp.ack.opcode == opcode:
			return True
		else:
			return False
