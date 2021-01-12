# Health App

## Download the HeMP2 Health App
1. Download the [HeMP2 Health App](https://modules-release.s3-us-west-2.amazonaws.com/health_windows_app/HeMP2_Buddy.zip)
2. Extract the files from the `HeMP2_Buddy` Zip Folder.
3. Move the `HeMP2_Buddy` Folder, if desired. This folder location will be know as [SLI-Modules]
### HeMP 2 Buddy Files
    |-- [SLI-Modules]\Hemp2_Buddy
        |-- HeMP2_Buddy.exe
        |-- LibUsbDotNet.dll
        |-- protobuf-net.dll

## Power Up the HeMP2 and Determine the CoMM Port
1. Connect a Micro-USB Cable to your PC and to the HeMP2 USB Port
2. Once powered, with the Red LED will blink at a steady 1 Hz rate, open the `Device Manager` by searching Windows
3. Under the `Ports (COM & LPT)`, note the COM Number of the new USB Device
    - For example `USB Serial Device (COM8`)

## Launch the HeMP2 Health App