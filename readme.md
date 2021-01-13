# SensorLogic NXP Xethru-X4 Ultra-wideband Radar Modules
The centerpiece is the 2-pc **He**alth **M**odule **P**CB (HeMP), which is comprised of a baseboard and an elliptical patch antenna module (EPAM). The baseboard foundation is an i.MX RT1060 crossover MCU from NXP with a host of environmental sensors recording temperature, humidity, lux, and noise pressure. The hallmark sensor is the X4 UWB radar that can be used for myriad applications including occupancy, proximity, and respiration, out to 10 meters (RCS depending).

## HeMP Health Firmware and App
The _Health Firmware_ runs our proprietary algorithms to identify the presence of, and distance to, a human target. If conditions are right, the human respiration is calculated within +/-1 RPM, out to 5 meters. The current room temperature, humidity, and illuminance levels are also calculated and stored. The data is transmitted via USB or Wi-Fi and is displayed on a basic C# Windows Forms App, the [HeMP Buddy](https://github.com/SensorLogicInc/modules/blob/module-initial-release/docs/health_app.md), ultimately displaying the subject's breathing pattern in real-time.

## HeMP MATLAB Firmware and Connector
The _MATLAB Firmware and Connector_ allows the user to use an efficient, high-level development environment, like MATLAB, to query the module for raw radar data for custom algorithm and application development. The data comes in two flavors, real RF data, effectively sampled at 23.328 GSps, or In-Phase/Quadrature (IQ) data that has been downconverted and decimated. Complete control of the radar is available by being able to query and set every radar register parameter. For example, changing parameters will affect the frame rate versus processing gain, depending on the application and required SNR.

## Folder Structure
```
├── docs
│   ├── health_app.md                   # Documentation about the HeMP2 Health App
│   ├── images/                         # Contains the images used in the markdown documents
│   ├── insecure_fw_update.md           # Documentation about updating the HeMP2 Firmware
│   └── readme.md
├── matlab
│   ├── images/                         # Contains the images used in the markdown documents
│   ├── readme.md                       # ReadMe describing how to use the VCOM XEP Radar Connector
│   ├── unit_test.m                     # MATLAB Script to verify the communication with the radar
│   ├── vcom_test.m                     # MATLAB Script to verify the ability to receive radar data
│   └── vcom_xep_radar_connector.m      # MATLAB Class to connect the Module to MATLAB
├── protocol_buffers
│   ├── hemp2_usb_vcom.options          # Health Firmware Protocol Buffer Options
│   ├── hemp2_usb_vcom.proto            # Health Firmware Protocol Buffer .proto file
│   └── readme.md                       # Health Firmware Protocol Buffer ReadMe
├── usb_driver
│   └── inf/                            # Contains the USB VCOM device driver for Windows 
├── .gitignore                          # Git Ignore
└── readme.md                           # This File
```
