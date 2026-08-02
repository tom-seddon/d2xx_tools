# d2xx_tools

`d2xx_tools` contains some basic command line tools for use on Windows
with FTDI serial interface devices. They use [FTDI's D2XX Direct
Drivers](https://ftdichip.com/drivers/d2xx-drivers/), and may work in
cases where the OS's ordinary serial port handling won't.

(I wrote these for use with BBC Micro stuff -
[UPURS](https://www.retro-kit.co.uk/UPURS/), or [a FT232H-based bus
snoop
device](https://stardot.org.uk/forums/viewtopic.php?f=3&t=14398) - but
in principle they're useable for anything.)

To install, get the latest version from
https://github.com/tom-seddon/d2xx_tools/releases/latest, and unzip
somewhere. Run a command prompt, and change to that directory.

There are 3 executables provided: `d2xx_list`, `d2xx_send`
and `d2xx_recv`.

## `d2xx_list`

`d2xx_list` lists the FTDI serial devices on your system. Run it
without any arguments: `d2xx_list`.

Example output from my computer:

```
2 devices:
Device 0:
 COM port: COM4
 Description: "FT232R USB UART"
 SerialNumber: A97DYC34
 (Type: 0x5 (FT_DEVICE_232R); Flags: 0x0; ID: 0x4036001; LocId: 0x1b)
Device 1:
 COM port: COM3
 Description: "Tube Serial M128 (FT4AHAA3)"
 SerialNumber: FT4AHAA3
 (Type: 0x8 (FT_DEVICE_232H); Flags: 0x2 - hi-speed; ID: 0x4036014; LocId: 0x1c4)
```

Hopefully the description and serial number will be enough to identify
the one you're thinking of. Note the COM port - this is what you'll
need to supply to the other tools to identify the device of interest.

If finds 0 devices when you'd expect it to find some:

- try rearranging the USB setup slightly. The fewer hubs involved, the
  better, and if it it works with one computer then it's still
  possible it might not work with another
- the D2XX driver doesn't support all possible FTDI devices anyway...
- some fake FTDI devices might not work with the D2xx driver

## `d2xx_send`

`d2xx_send` sends file contents over the serial device, by default
with UPURS-compatible settings: 115,200 baud, 8 bits, no parity, 1
stop bit, RTS/CTS flow control.

Run it with 2 arguments: firstly the COM port to use (see the
`d2xx_list` section), and secondly the file to send. For example:
`d2xx_send com4 D:\temp\test.dat`.

Once the entire file has been sent, `d2xx_send` will exit.

### advanced `d2xx_send` stuff

`d2xx_send` has various options for configuring the serial device
properties. Run `d2xx_send --help` to get a summary.

You can supply multiple file names. They will be sent as a single
stream of bytes, one after the other, in the order given.

## `d2xx_recv`

`d2xx_recv` receives file contents from the serial device, by default
with UPURS-compatible settings: 115,200 baud, 8 bits, no parity, 1
stop bit, RTS/CTS flow contrnol.

Run it with 2 arguments: firstly the COM port to use (see the
`d2xx_list` section), and secondly the file to write the data to . For
example: `d2xx_send com4 D:\temp\test.dat`.

(If the supplied file exists, it will be overwritten, no questions
asked!)

As data is received, the number of bytes received will be updated.
`d2xx_recv` has no way of knowing how many bytes are expected, so
you'll have to watch its output and monitor the sending device's
status. Once the transfer is done, press any key, and `d2xx_recv` will
exit.

### advanced `d2xx_recv` stuff

`d2xx_recv` has various options for configuring the serial device
properties. Run `d2xx_recv --help` to get a summary.

# build it yourself

See [the build instructions](./docs/build.md).

# licence

`src` - GPL v3. See [COPYING.txt](./COPYING.txt).

`submodules`, `dependencies` - see individual folders
