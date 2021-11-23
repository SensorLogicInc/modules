# Insecure Firmware Update Guide

[Back](readme.md)

This is a guide on how to manually update the firmware on a SLMX4-Base using the NXP-MCUBootUtility tool.

## Downloads
- [NXP-MCUBootUtility Tool](https://github.com/JayHeng/NXP-MCUBootUtility/releases): Instructions using this tool shown below
- [MCUXpresso Secure Provisioning Tool](https://www.dropbox.com/sh/625jy4ovpvaticd/AAAo25quFGzx2itQyhBfPT63a?dl=0): Alternate method to update SLMX4 firmware

## Using the NXP-MCUBootUtility Tool

### Step 1: Set the boot pins

Before we can use the tool, we need to set the boot pins on the SLMX4-Base itself.
When using the NXP-MCUBootUtility, set the SW4 pins to match the image on the
left, `Firmware Update Mode`.

Once the firmware has been updated, set the SW4 pins to match the image on the
right, `Normal Settings Mode`.

|Firmware Update|Normal Settings|
|-|-|
|![](../images/firmware_update/slmx4_base_fw_update_boot_pins.jpg)|![](../images/firmware_update/slmx4_base_normal_boot_pins.jpg)|

Once the boot pins are set, plug in the USB cable to power the SLMX4-Base. When
the boot pins are set in the `Firmware Update Mode`, the RGB LED will display a violet color.
 
### Step 2: Start NXP-MCUBootUtility

When you start the NXP-MCUBootUtility for the first time, you'll need to make
sure to select the correct NPX device. This will populate the vendor and product
ID values in the 'Port Setup' area.

When the program starts, there will also be a text console window open. It will
not have any content initially.

![](../images/firmware_update/boot_util_1.png)

Next, click the 'Connect to ROM' button. This may take a few seconds, but there
will a flurry of activity in the text window, and eventually, the main window
will change.

![](../images/firmware_update/boot_util_2.png)

### Step 3: Update the Firmware

In the green box, click the 'Browse' button and select the `.s19` firmware file
you wish to update. Since the firmware files are `.s19` files, choose the option
for 'Motorola S-Records (.srec/.s19)'.

![](../images/firmware_update/boot_util_3.png)

Next, click the 'All-In-One Action' button just above. The main window will then
change a few times.

![](../images/firmware_update/boot_util_4.png)

After a few seconds, the window will change again and update as the firmware is
flashed on to the SLMX4-Base.

![](../images/firmware_update/boot_util_5.png)

Depending on the size of the firmware file, it can take a while to finish. Once
the update is done, the program will play a sound. The progress bar will also be
completely full.

### Step 4: Reset the Boot Pins

Close the program and then unplug the USB cable from the SLMX4-Base.

Next, reset the boot pins for normal operation, then plug the USB in again. The
SLMX4-Base should then boot normally and execute whatever firmware which was 
updated.

## Using the MCUXpresso Secure Provisioning Tool

### Step 1: Download the Correct Installer

Download the appropriate tool for your OS from this Dropbox Link: [MCUXpresso Secure Provisioning Tool](https://www.dropbox.com/sh/625jy4ovpvaticd/AAAo25quFGzx2itQyhBfPT63a?dl=0)

![](../images/MCUXpresso_firmware_update/1.png)

### Step 2: Install the Software (WinOS shown)

Click on the "MCUXpresso_Secure_Provisioning_v3.1.exe" file to install.

![](../images/MCUXpresso_firmware_update/3.png)

Close the installer and launch the MCUXpresso Secure Provisioning v3.1 software.

![](../images/MCUXpresso_firmware_update/4.png)

### Step 3: Software Setup

The first time you run the provisioning tool, you will be greeted with this window:

![](../images/MCUXpresso_firmware_update/5.PNG)

The current SLMX4 uses the MIMXRT106S and so you will make this selection:

![](../images/MCUXpresso_firmware_update/6.PNG)

Click "Create" and this window will appear:

![](../images/MCUXpresso_firmware_update/7.PNG)

### Step 4: Set the boot pins

Before programming the board with the MCUXpresso Secure Provisioning Tool, set the boot pins (SW4) on the SLMX4-Base to `Firmware Update Mode`, same as [Step 1](#using-the-nxp-mcubootutility-tool) above.

![](../images/firmware_update/slmx4_base_fw_update_boot_pins.jpg)

Once the boot pins are set, plug in the USB cable to power the SLMX4-Base. When
the boot pins are set in the `Firmware Update Mode` the RGB, Green, and Red LEDs will turn on.

![](../images/MCUXpresso_firmware_update/8.png)

### Step Five: Flash the SLMX4 with the Firmware

Next, select an image to flash onto the SLMX4. Select "Browse..." next to the "Source executable image:" and select the .s19 file you wish to flash.
If the "Configuration Helper" pop-up appears, click on "Deselect All" and click "OK".

![](../images/MCUXpresso_firmware_update/9.png)

Next, for the "DCD (binary)" option, select "From Source Image" from the dropdown menu. Select "Build Image"

![](../images/MCUXpresso_firmware_update/10.png)

This window will appear. Ensure that the build was successful. You may close this mini-window and click on the "Write image" tab

![](../images/MCUXpresso_firmware_update/11.png)

From here, select "Write Image" and wait for the process to complete.

![](../images/MCUXpresso_firmware_update/12.png)

Ensure that the process is successful. You may close this mini-window. The SLMX4 is now flashed with your selected firmware.

![](../images/MCUXpresso_firmware_update/13.png)

Finally, unplug the unit and ensure the SW4 pins are back to `Normal Settings Mode`, as seen in [Step 1](#using-the-nxp-mcubootutility-tool) above, then power-up the unit.
