# Protocol Buffers
[Protocol Buffers](https://developers.google.com/protocol-buffers) provides a
convenient mechanism for encoding/decoding serialized data across multiple
hardware platforms and multiple software languages.

On the SLMX4, protocol buffers is used for the **Health** firmware for USB
communications.

## Generating Protocol Buffers in C
On the SLMX4, the firmware is written in C. The tool to generate the `.c` and `.h`
files from the `.proto` and `.options` file is [nanopb](https://jpa.kapsi.fi/nanopb/).

```
# ./protoc -oslmx4_usb_vcom.pb slmx4_usb_vcom.proto
# python ../nanopb/generator/nanopb_generator.py slmx4_usb_vcom.pb
```

## Generating Protocol Buffers in other languages
In more modern languages, we've used an [online generator](https://protogen.marcgravell.com/)
with success.

With this tool, you paste in the contents of the `.proto` file then click the
'Generate' button.

## Errata
We've used protocol buffers over a number of hardware transports. To make things
simple, we include the length of the data in advance of the data. The length is
set to be a fixed length 32-bit unsigned integer. The data is an array of uint8.

The format is:  
```
[len (uint32)][data]
```

## SLMX4 Health proto
This [link](slmx4_health.md) has more info on how a subset of commands are
encoded.

The [`.proto`](slmx4_usb_vcom.proto) file is used to generate the language-specific
code. In C, we also use the [`.options`](slmx4_usb_vcom.option) file which is needed
to set the array sizes for various fixed length arrays; this may not be needed in
other languages, but simplifies things in C.
