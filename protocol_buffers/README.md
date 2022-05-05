# Protocol Buffers

[Back](../)

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

## [SLMX4 Health Proto](slmx4_health.md)
This [link](slmx4_health.md) has more info on how a subset of commands are
encoded.

The [`.proto`](slmx4_usb_vcom.proto) file is used to generate the language-specific
code. In C, we also use the [`.options`](slmx4_usb_vcom.options) file which is needed
to set the array sizes for various fixed length arrays; this may not be needed in
other languages, but simplifies things in C.

> Health-specific Protobuf files

- [`.proto`](slmx4_usb_vcom.proto)
- [`.options`](slmx4_usb_vcom.options)

### C# Client Example of Protobuf Implementation

```
// This event triggers when USB VCOM data is received...
private void serialPort1_DataReceived(object sender, SerialDataReceivedEventArgs e)
{
	//Debug.WriteLine(serialPort1.ReadExisting());
	int len = serialPort1.BytesToRead;
	byte[] toAdd = new byte[len];
	serialPort1.Read(toAdd, 0, len);
	try
	{
		rxList.AddRange(toAdd);
	}
	catch
	{
		// do nothing
	}
}

private void HandleMessagesThread()
{
	while (true)
	{
		try
		{
			ProcessReceived(rxList);
		}
		catch (OverflowException)
		{
			Debug.WriteLine("Overflow exception: clearing receive buffer!");
			rxList.Clear();
		}
		catch (Exception se)
		{
			Debug.WriteLine("HandleMessages():");
			Debug.WriteLine(se.Message);
			Debug.WriteLine(se.StackTrace);
		}
	}
}

private void ProcessReceived(List<byte> list)
{
	byte[] buffer;
	try
	{
		buffer = list.ToArray();
	}
	catch (ArgumentException ex)
	{
		Debug.WriteLine("ProcessReceived():");
		Debug.WriteLine(ex.Message);
		Debug.WriteLine(ex.StackTrace);
		return;
	}

	// Get the length of the buffer
	int bytesToProcess = buffer.Length;

	int i = 0;
	while (i < bytesToProcess)
	{
		int numToRead = 0;
		try
		{
			numToRead = BitConverter.ToInt32(buffer, i);
		}
		catch (ArgumentException)
		{
			Debug.WriteLine("BitConverter exception...");
			return;
		}
		//Debug.WriteLine("num to read = " + numToRead);
		byte[] bytesToDecode = new byte[numToRead + 4];

		// If the data indicates there is more data to be read than exists in the list, skip
		// this round until there is more data. This is related to the MTU of TCP. The radar
		// data exceeds 1460 bytes. This if-statement added as a bug fix.
		if (bytesToDecode.Length > bytesToProcess) break;

		try
		{
			// Copy the number of bytes to read from buffer into the array to decode, increment the byte counter
			Array.ConstrainedCopy(buffer, i, bytesToDecode, 0, bytesToDecode.Length);
			i += bytesToDecode.Length;

			// Remove the read bytes from the received bytes list
			list.RemoveRange(0, bytesToDecode.Length);

			//Debug.WriteLine("read " + numToRead + " bytes, opcode = " + bytesToDecode[5].ToString("x"));

			MemoryStream stream = new MemoryStream(bytesToDecode);
			serverResponse = Serializer.DeserializeWithLengthPrefix<ServerResponseT>(stream, PrefixStyle.Fixed32);

			if (serverResponse == null) continue;

			switch (serverResponse.Opcode)
			{
				case Opcode.Ack:
					Debug.WriteLine("got ACK for opcode = " + serverResponse.Ack.Opcode.ToString());
					break;

				case Opcode.StatusMsg:
					if (serverResponse.Str != null)
					{
						MsgPayloadStrT msg = null;
						lock (syncObj)
						{
							msg = serverResponse.Str;
						}
						statusLabel.Text = msg.Str;
						Debug.WriteLine("status msg = " + msg.Str);

						if (msg.Str == "Presence calibration starting...")
						{
							pcalPanel.BackColor = Color.Cyan;
						}

						if (msg.Str == "Presence calibration complete...")
						{
							pcalPanel.BackColor = defaultControlColor;
						}
					}
					break;

				case Opcode.HealthMsg:
					if (serverResponse.Health != null)
					{
						MsgPayloadHealthT health = null;
						lock (syncObj)
						{
							health = serverResponse.Health;
						}
						//statusLabel.Text = serverResponse.Health.Debugs[0].ToString();
						UpdateHealth(health);
					}
					break;

				case Opcode.OneShot:
					if (serverResponse.Vector != null)
					{
						MsgPayloadVectorT respwave = null;
						lock (syncObj)
						{
							respwave = serverResponse.Vector;
						}
						UpdatePlot(respwave);
					}
					break;

				case Opcode.LogData:
					if (serverResponse.LogData != null)
					{
						MsgPayloadLogDataT datum = null;
						lock (syncObj)
						{
							datum = serverResponse.LogData;
						}
					}
					break;

				case Opcode.Version:
					string verRaw = serverResponse.Str.Str;
					string[] ver = verRaw.Split(',');

					if (ver.Length == 1)
					{
						versionLabel.Text = "pb ver: " + serverResponse.Str.Str;

						// Probably want to solve this better but old version didn't support binary logs
						// so make sure to forcefully disable the checkbox
						if (serverResponse.Str.Str == "1.8.0")
						{
							logCheckBox.Enabled = false;
						}
					}
					else
					{
						string fwName = "fw: " + ver[0] + " v" + ver[1];
						string pbVer = "pb: " + ver[2];

						versionLabel.Text = fwName + "   " + pbVer;

						// Probably want to solve this better but old version didn't support binary logs
						// so make sure to forcefully disable the checkbox
						if (ver[2] == "1.8.0")
						{
							logCheckBox.Enabled = false;
						}

						if (ver[1] != "1.0.0")
						{
							healthVersion = 4;
						}
					}
					break;

				default:
					break;
			}

			serverResponse = null;
		}
		catch (Exception ex)
		{
			Debug.WriteLine("ProcessReceived():");
			Debug.WriteLine(ex.Message);
			Debug.WriteLine(ex.StackTrace);
			Debug.WriteLine("~~ numToRead = " + numToRead);
			Debug.WriteLine("~~ bytesToDecode len = " + bytesToDecode.Length);
			Debug.WriteLine("~~ buffer len = " + buffer.Length);
			Debug.WriteLine("~~ i = " + i);
			return;
		}
	}
}

private void SendData(byte[] data)
{
	try
	{
		serialPort1.Write(data, 0, data.Length);
	}
	catch
	{
		// timeout
	}
}

// This is a sample of protobuf encoding...
private void SendSerialCmd(string msg)
{
	command.ResetEmpty();
	command.Opcode = Opcode.SerialCmd;
	command.ShouldSerializeStr();
	command.Str = new MsgPayloadStrT();
	command.Str.Str = msg;

	byte[] commandBytes = SerializeClass(command);
	string bytesToHexString = "";
	foreach (byte b in commandBytes)
	{
		bytesToHexString += "0x" + b.ToString("x") + " ";
	}
	Debug.WriteLine("Command as Hex: " + bytesToHexString);
	SendMessage(command);
}

private void SendMessage<T>(T message) where T : class
{
	SendData(SerializeClass(message));
}
```
