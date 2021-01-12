# SensorLogic NXP-Based X4 Ultra-Wide Band Radar Modules

## HeMP2 Health Firmware and App

## HeMP2 MATLAB Firmware and Connector

## Folder Structure
```
├── docs
│   ├── Health_App.md					# Documentation about the HeMP2 Health App
│   ├── images/							# Contains the images used in the markdown documents
│   ├── insecure_fw_update.md			# Documentation about updating the HeMP2 Firmware
│   └── README.md
├── matlab
│   ├── images/							# Contains the images used in the markdown documents
│   ├── README.md						# ReadMe describing how to use the VCOM XEP Radar Connector
│   ├── unit_test.m						# MATLAB Script to verify the communication with the radar
│   ├── vcom_test.m						# MATLAB Script to verify the ability to receive radar data
│   └── vcom_xep_radar_connector.m		# MATLAB Class to connect the Module to MATLAB
├── protocol_buffers
│   ├── health_protocol_buffer
│   │   └── README.md
│   ├── hemp2_usb_vcom.options
│   ├── hemp2_usb_vcom.proto
│   ├── matlab_protocol_buffer
│   │   └── README.md
│   └── README.md
├── .gitignore
└── README.md							# This File
```