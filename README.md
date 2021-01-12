# SensorLogic NXP Xethru-X4 Ultra-wideband Radar Modules
The centerpiece is the 2-pc **He**alth **M**odule **P**CB, which is comprised of a baseboard and an elliptical patch antenna module (EPAM). The baseboard foundation is an i.MX RT1060 crossover MCU from NXP with a host of environmental sensors recording temperature, humidity, lux, and noise pressure. The hallmark sensor is the X4 UWB sensor that can be used for myriad applications including occupancy, proximity, and respiration, out to 10 meters (RCS depending).

## HeMP Health Firmware and App
The _Health Firmware_ runs our proprietary algorithms to identify the presence of, and distance to, a human target. If conditions are right, the repiration of the human is calculated within +/-1 RPM. The current room tempature, humidity, and illuminance levels are also calculated and stored. The data is transmitted via USB or Wi-Fi, depending on the application and is displayed on a basic C# Windows Forms app, HeMP Buddy, ultimately displaying the subject's breathing pattern.

## HeMP MATLAB Firmware and Connector
The _MATLAB Firmware and Connector_ allow the user to use an efficient, high-level developmement environment, like MATLAB, to query the module for raw radar data for custom algorithm and application development. The data comes in two flavors, real RF data effectively sampled at 23.328 GSps, or In-Phase/Quadarature (IQ) data that has been downconverted and decimated. Complete control of the radar is available by being able to query and set every radar register paramater. For example, changing parameters will affect the frame rate vs. processing gain, depending on the application required SNR.

## Folder Structure
```
├── docs
│   ├── Health_App.md                   # Documentation about the HeMP2 Health App
│   ├── images/                         # Contains the images used in the markdown documents
│   ├── insecure_fw_update.md           # Documentation about updating the HeMP2 Firmware
│   └── README.md
├── matlab
│   ├── images/                         # Contains the images used in the markdown documents
│   ├── README.md                       # ReadMe describing how to use the VCOM XEP Radar Connector
│   ├── unit_test.m                     # MATLAB Script to verify the communication with the radar
│   ├── vcom_test.m                     # MATLAB Script to verify the ability to receive radar data
│   └── vcom_xep_radar_connector.m      # MATLAB Class to connect the Module to MATLAB
├── protocol_buffers
│   ├── health_protocol_buffer
│   │   └── README.md
│   ├── hemp2_usb_vcom.options
│   ├── hemp2_usb_vcom.proto
│   ├── matlab_protocol_buffer
│   │   └── README.md
│   └── README.md
├── .gitignore
└── README.md                           # This File
```
