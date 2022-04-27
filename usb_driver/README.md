# USB Driver Install

[Back](../)

This guide contains steps needed to install the USB driver when Windows doesn't
automatically install it.

Open the Windows Device Manager. The SLMX4 will display as **MCU VIRTUAL COM DEMO**.

<p align="center">
  <img src="../images/usb/usb_driver1.png" />
</p>

Right-click and select 'Update Driver Software...'.

<p align="center">
  <img src="../images/usb/usb_driver2.png" />
</p>

When the new window opens, select the bottom button 'Browse my computer...'.

<p align="center">
  <img src="../images/usb/usb_driver3.png" />
</p>

Browse for the location of the `usb_driver/inf` folder. In this case, this shows
the indicated folder where I had checked out the github repository.

<p align="center">
  <img src="../images/usb/usb_driver4.png" />
</p>

During the installation there will be a Windows Security window.

<p align="center">
  <img src="../images/usb/usb_driver5.png" />
</p>

After some time, the driver should install successfully.

<p align="center">
  <img src="../images/usb/usb_driver6.png" />
</p>

When we hit 'Close' our SLMX4 device will now show up under the **Ports** 
category. In this case, it's COM106 on this test PC.

<p align="center">
  <img src="../images/usb/usb_driver7.png" />
</p>

In this last image, the USB VID and PID are shown. This could be added to any
user app to distinguish the SLMX4 from other virtual COM devices.

<p align="center">
  <img src="../images/usb/usb_driver8.png" />
</p>
