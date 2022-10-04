# SLMX4 Health Python Wrapper

[Back](../)

> This folder contains python code for interaction with the SLM-X4 module running the
  [health firmware](https://www.dropbox.com/s/nkkn8px5ii1hrya/slmx4_base_usb_vcom_pb_dsp-epam0P1.s19?dl=1).

The [slmx4_health_wrapper.py](slmx4_health_wrapper.py) file contains an interface which allows 
python to interact with the SLM-X4 running the health firmware.

## Health Firmware Protocol

> The health firmware that the python wrapper uses operates on the USB port as a Virtual COM port. 
  On a Windows system, the device will show up in the 'Device Manager' with an identification such
  as 'COM6'. On Linux, the identification will typically be `/dev/ttyACM0`.

The health firmware uses USB as the physical interface. On top of that, the health firmware passes
messages using Protocol Buffers. The [`slmx4_usb_vcom_pb2.py`](slmx4_usb_vcom_pb2.py) file contains
the specific protocol buffer specification in python.

> The `slmx4_usb_vcom_pb2.py` specification includes definitions used outside of the health firmware.
  The included wrapper accesses the necessary functions to operate the health firmware only!

## Python Dependencies

The python wrapper uses mostly standard python3 modules. However, since the USB port is used we need
a serial interface, `pyserial` in this case. Installation using pip is the same on Windows and Linux.

> On Windows you may need to have admininistrative privileges...

To install `pyserial`:
```
python3 -m pip install pyserial
```

There is also a demo which plots the respiration waveform. This requires use of `matplotlib`.

To install `matplotlib`:
```
python3 -m pip install -U matplotlib
```

The final dependency is protocol buffers itself.

### Windows - Protocol Buffers

1. Download the appropriate precompiled version of proto from this [link](https://github.com/protocolbuffers/protobuf/releases)
   and unzip the folder to some location on your PC. 
2. Add the path to the folder's bin directory to your installation's Environmental Variables.
3. Clone the [protocol buffers](https://github.com/protocolbuffers/protobuf) repository.
3. In the python folder  
   ```
   python3 setup.py install
   ```

### Linux - Protocol Buffers

To install protocol buffers on Ubuntu Linux:
```
sudo apt update
sudo apt install protobuf-compiler
```

## Python Demos

There are a number of example python demonstrations.

- [slmx4_health_polling_demo.py](slmx4_health_polling_demo.py)  
  The demonstrates how to manually trigger (or poll) the radar on the SLM-X4. _This is not recommended
  to due to measurement time jitter which may affect respiration calculations._
  
- [slmx4_health_streaming_demo.py](slmx4_health_streaming_demo.py)  
  This demonstrates how data streaming works. The SLM-X4 once data streaming starts will generate
  and process radar data at a fixed rate with little jitter.
  
- [slmx4_health_streaming_plot_demo.py](slmx4_health_streaming_plot_demo.py)  
  This is similar to the streaming demo except the respiration waveform is plotted in addition to
  the data shown on the terminal

> The python demos have been tested on Ubuntu Linux 20.04 LTS and Windows 10.
