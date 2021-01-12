# SensorLogic NXP-Based X4 Ultra-Wide Band Radar Modules

## HeMP2 Health Firmware and App

## HeMP2 MATLAB Firmware and Connector

## Folder Structure
    |-- docs/
        |-- Health_App.md                   # Documentation about the HeMP2 Health App
        |-- images/                         # Contains the images used in the markdown documents
        |-- Insecure_Update.md              # Documentation about updating the HeMP2 Firmware
    |-- matlab/
        |-- images/                         # Contains the images used in the markdown documents
        |-- README.md                       # ReadMe describing how to use the VCOM XEP Radar Connector
        |-- unit_test.m                     # MATLAB Script to verify the communication with the radar
        |-- vcom_test.m                     # MATLAB Script to verify the ability to receive radar data
        |-- vcom_xep_radar_connector.m      # MATLAB Class to connect the Module to MATLAB
    |-- protocol_buffers/
        |-- health_protocol_buffer/
            |-- README.md                   # ReadMe describing the Health Firmware Protocol Buffer
        |-- matlab_protocol_buffer/
            |-- README.md                   # ReadMe describing the MATLAB Firmware Protocol Buffer
        |-- README.md
    |-- .gitignore                          # Git Ignore File
    |-- README.md                           # This File
