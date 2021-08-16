# magboot
Magboot is a bootloader for the Atmega-family of Atmel processors. It provides
the ability to write the onboard flash memory over the serial port. It can be
used to flash an AVR device with new firmware without a dedicated ISP programmer
device.

## Features
* Flash firmware over the serial port (no need for a dedicated ISP programmer)
* Supports Atmega168(p) and Atmega328(p)
* Basic device identification to prevent flashing unknown or unsupported devices
* Fits in a 1 kb boot section (binary size is approx. 700 bytes)
* Supports loading raw binaries only (ihex is NOT supported at the moment)
* Supports native AVR serialport or software serialport on two arbitrary pins
* Data transfers over the serial port are verified to prevent flashing corrupt
  data.

## Building and flashing
To install magboot you need an ISP programmer.

1) Edit Makefile and the "Target definition"-section:

```
MCU         Set to either 'atmega328p' or 'atmega168p' depending on target.

HFUSE       Sets AVR Hi-Fuse. Normally you will not have to change this
            value for either 328p or 168p. If you have special requirements,
            http://www.engbedded.com/fusecalc is a good AVR fuse calculator.

BOOTADDR    This is flash size minus boot section size, expressed in bytes.
            For example, 32 kb - 1 kb = 31 kb = 31 * 1024 = 0x7c00
            Set to 0x7c00 for Atmega328p and 0x3c00 for Atmega168p.
```

2) Edit the target configuration in config.h:

```
FCPU        Target clock frequency
```

Unless you plan to use the software serialport implementation (SWUART), you do
not have to change any of the CFG_SWUART_* directives.

3) Build and program the device:

```
make magboot_hw
make flash_hw
```

The Makefile assumes your ISP programmer is of type stk500v2 and located at file
/dev/avrusb0. If this is not true, append the PROGTYPE and PROGPORT parameters
with appropriate values. Example:

```
make PROGPORT=/dev/ttyUSB0 PROGTYPE=jtagmkII flash_hw
```

For a list of valid PROGTYPE values, please refer to the
[Avrdude manual](http://www.nongnu.org/avrdude/user-manual/avrdude_4.html).

Scroll down to the -c option for a complete list of supported programmers.

## Usage
Magboot ships with a python-script which handles the serial communication with
the device running the bootloader. It accepts a sequence of commands which are
executed in the same order as they appear on the command line.

Review magboot.py --help for a list of available commands.

If the bootloader receives no command in 4 seconds, it will jump the application
address. To re-enter the bootloader, reset your device and execute your command
sequence within four seconds. Please note that reset implies an external reset
(pulling the RESET pin low).

Power-cycling, resetting using watchdog or if a brown-out occurs, the bootloader
will be bypassed upon next reboot. The philosophy behind this is that during
normal reset conditions, the bootloader should not interfere. It will ONLY enter
its programming mode if someone hits the RESET button.

The python script depends on the python serial module, which is prepackage on
most Linux distros. On Debian/Ubuntu, use apt-get install python-serial to
install it.

### Example #1 - Check if magboot is alive and responding
The device identification command will ask the device to compare a known
signature to its own signature.

```
./magboot.py /dev/ttyUSB0 atmega328p -i
```

It can also be used to check for device presence. If no reply is received in
a few seconds, the python script will print a timeout error. If this happens,
try resetting your device to make sure the 4 second timer has not expired.

### Example #2 - Program the testapp
Included with magboot is a testprogram. It is a very simple program which
flashes a LED at 2 Hz (connected on Port B, PB5). Begin by building the testapp:

```
make -C testapp
```

Proceed by uploading it using the python-script:

```
./magboot.py /dev/ttyUSB0 atmega328p -z -w testapp/testapp.bin -r
```

Magboot will execute the following commands:
- Wait for device to appear on /dev/ttyUSB0
- Write file 'testapp/testapp.bin' to beginning of flash memory
- Reset the device

If your device starts flashing at 2 Hz, the operation was successfull!

## Software serial port support (ADVANCED)
Magboot normally operates over the built-in serialport of the AVR using the
predefined TX and RX pins. This covers most needs and is the recommended mode of
operation unless your serial port is connected to another pair of pins.

In this case, you may use the software serialport mode of operation. It allows
you to configure two arbitrary pins to use for TX and RX. This is useful if the
native serialport is unavailable, perhaps because of other peripheral serial
devices.

The serialport mode is a compile-time option. You may build the SoftWare UART
(SWUART) version of Magboot by issuing Make-target "magboot_sw":

```
make magboot_sw
```

Prior to building the SWUART version, pin-configuration must be setup in
config.h. Please note that all the configuration steps listed in the
Installation section above also applies for the SWUART build (F_CPU etc.).

Finally, flash Magboot SWUART onto target:

```
make [PROGPORT=/dev/ttyUSB0 PROGTYPE=myprogrammer] flash_sw
```

For usage information, please refer to the Usage section above.
