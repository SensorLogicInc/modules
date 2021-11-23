# SLMX4 Projects
This repository contains various firmware projects for the SensorLogic SLMX4
module.

These projects require the use of the [MCUXpresso](https://www.nxp.com/design/software/development-software/mcuxpresso-software-and-tools-/mcuxpresso-integrated-development-environment-ide:MCUXpresso-IDE?tab=Design_Tools_Tab) IDE and an appropriate [JTAG emulator](https://www.nxp.com/design/microcontrollers-developer-resources/lpc-link2:OM13054).

## Setup MCUXpresso

Once MCUXpresso is installed, when the program is started it will ask the user
to create a workspace. _The location isn't critical, except when new projects
will be created in the future, MCUXpresso will use the workspace folder as the
default._

Download the [NXP SDK](https://www.dropbox.com/s/mu2ej0ns24gxljw/SDK_2_10_0_EVK-MIMXRT1060.zip?dl=0)
 and drag the file into the 'Installed SDKs' tab at the bottom of the Window.
 
To import a project, choose 'File->Open Projects from File System...' and
in the 'Import source:' text box, select the folder where the code repository
was downloaded to. Here, you can select all, or a just of subset of projects
to import.

## Projects  
- **[button_demo](button_demo)**  
  This demonstrates using user buttons to execute actions based on using a 
  FreeRTOS "one-shot" timer
- **[button_demo2](button_demo2)**  
  This demonstrates using user buttons to execute actions based on how long the
  button is pressed for.
- **[led_blink](led_blink)**  
  This demonstrates how to use the LEDs on the SLMX4.
- **[lux_demo](lux_demo)**  
  This demonstrates how to use the LUX sensor on the SLMX4.
- **[vcom_xep_matlab_server](vcom_xep_matlab_server)**  
  This exposes the USB on the SLMX4 as a virtual COM part and instantiates a server that will commuincate with the 
  [vcom_xep_radar_connector](../matlab/vcom_xep_radar_connector.m) client, using MATLAB. The Xethru `xep` driver enables full communication, 
  parameter configuration, and data streaming to/from the X4 radar SoC, the primary sensing technology on the SLMX4.
