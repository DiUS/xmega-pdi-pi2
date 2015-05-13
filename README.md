xmega-pdi-pi2
=============

Programmer for flashing a directly connected AVR XMEGA from a Raspberry
Pi 2, using the PDI protocol.


Rationale
---------

Normally all programming of XMEGA chips is done via a separate programmer,
such as the AVRISPmkII. When the XMEGA is directly connected to a
Raspberry Pi, such as on our RPi-LSR-HAT, there is a need to be able to
program it from said Pi, both for initial firmware installation as well
as upgrades.

Due to the hard timing constraints imposed by the PDI protocol, this
typically necessitates adding a separate micro processor to perform the
actual programming (which is just what the external programmers are).
Thanks to the Raspberry Pi 2 being a multicore chip, it is possible
to talk PDI straight to the XMEGA without losing sync. This feat does
however require boosting the flashing process to realtime priority and
effectively it uses an entire core while engaging in PDI.


Features
--------

The xmega-pdi-pi2 tool provides the following features:

  - Chip erase functionality
  - Flashing (page erase+write) of application & boot areas
  - Dumping existing flash content
  - Intel HEX input file support (.ihex files)
  - Configurable GPIO selection
  - Configurable flash base address


Usage
-----

```
syntax: ./pdi [-h] [-q] [-a baseaddr] [-b] [-c clkpin] [-d datapin] [-s pdidelay] [-D len@offs] [-E] [-F ihexfile]

  -q             quiet mode
  -a baseaddr    override base address (note: PDI address space)
  -b             use default boot flash instead of app flash address
  -c clkpin      set gpio pin to use as PDI_CLK
  -d datapin     set gpio pin to use as PDI_DATA
  -s pdidelay    set PDI clock delay, in us
  -D len@offs    dump memory, len bytes from (baseaddr + offs)
  -E             perform chip erase
  -F ihexfile    write ihexfile
  -h             show this help
```

Length and offset values for dumping memory can be given in decimal or
hexadecimal (or octal, but why would you?!). Using a non-default `pdidelay`
value should not be necessary. The default flash base address should be
sensible for all XMEGAs, but the address picked by the `-b` option is
only applicable to the XMEGA256. You probably need to use the `-a` option
with the correct value for other XMEGAs.

A minimum of 25% realtime ratio available, as
defined by /proc/sys/kernel/sched_rt_period_us and
/proc/sys/kernel/sched_rt_runtime_us. The tool needs to have a core
for itself, uninterrupted, while it's talking PDI.


Examples
--------

Our application comprises a seperate boot loader and the main application.
Both are available as Intel HEX format, generated from objdump(1), and
named "bootloader.ihex" and "main.ihex".

We have an XMEGA256A3 directly connected to the Pi 2. The PDI_CLK line
is connected to GPIO24 (J8 pin 18), and PDI_DATA is on GPIO21 (J8 pin 40).

Erasing chip and installing the bootloader:
```
# ./pdi -c 24 -d 21 -b -E -F bootloader.ihex
Using: clk=gpio24, data=gpio21, delay: 0us, baseaddr: 0x00840000 (boot-flash)
Actions: chip-erase program:bootloader.ihex 
ok
#
```

Installing the main application (note that we do *not* perform a chip erase
here, as that would erase the bootloader). Note that this takes a few
seconds to complete.
```
# ./pdi -c 24 -d 21 -F main.ihex
Using: clk=gpio24, data=gpio21, delay: 0us, baseaddr: 0x00800000 (app-flash)
Actions: program:main.ihex 
ok
#
```

Verifying what we wrote to the application flash:
```
# ./pdi -c 24 -d 21 -D 64@0
Using: clk=gpio24, data=gpio21, delay: 0us, baseaddr: 0x00800000 (app-flash)
Actions: dump-memory 
00000000: 0c 94 3e 03 0c 94 5f 03  0c 94 b3 8d 0c 94 cd 8d 
00000010: 0c 94 a7 8e 0c 94 c1 8e  0c 94 5f 03 0c 94 5f 03 
00000020: 0c 94 5f 03 0c 94 5f 03  0c 94 6b 8a 0c 94 d0 8a 
00000030: 0c 94 5f 03 0c 94 5f 03  0c 94 17 8b 0c 94 5f 03 
ok
#
```

Since the default GPIOs selected are 24 and 21 for CLK/DATA respectively, the
`-c` and `-d` options shown above weren't strictly needed. In this case
it's possible to use a much shorter invocation, such as:
```
# ./pdi -D 13@0x4437
Using: clk=gpio24, data=gpio21, delay: 0us, baseaddr: 0x00800000 (app-flash)
Actions: dump-memory 
00004430:                      88  fc 01 60 81 71 81 6e 3f 
00004440: 70 48 09 f0 
ok
#
```


Building
--------

Required tools and libraries:

  - libcm2835 (available from http://www.airspayce.com/mikem/bcm2835)
  - C compiler, GNU make  (apt-get install build-essential)
  - C++ compiler (apt-get install g++)


Building the xmega-pdi-pi2 tool is as simple as extracting the source
and typing 'make' in the extracted directory.


Known limitations
-----------------

  - Only tested with XMEGA256A3
  - Assumes a flash page size of 512 bytes
  - No support for writing to EEPROM
  - No support for FUSEs

