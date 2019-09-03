# Systronix_LCM300
Arduino library for the Artesyn LCM300 family of 24/36/48V supplies with PMBus, intended for use with Teensy. Some simple and useful examples are included, which print out basic data about the connected supply.

## Required libraries
- i2c_t3 special I2C library for Teensy. We have forked this so for this repo use [our fork](https://github.com/systronix/i2c_t3)
- Systronix I2C common library[Systronix_i2c_common](https://github.com/systronix/Systronix_i2c_common)

## What is PMBus?
- Power Management Bus. Based on I2C. Lets you monitor and control a power supply from almost any microcontroller. Great idea, right? We think so.
- [PMBus at wikipedia](https://en.wikipedia.org/wiki/Power_Management_Bus)
- [Lawsuit between Power-One and Artesyn](http://electronicdesign.com/boards/patent-lawsuit-verdict-clouds-future-pmbus) 2008 article by Sam Davis. why can't we all just get along?
- [PMBus - panacea or hype?](https://web.archive.org/web/20110718095312/http://www2.electronicproducts.com/PageSearch.aspx?FName=artesyn.aug2005.HTML) 2011 article written by Bob White of Artesyn, one of the PMBus founders.
- [Power One is now part of Bel Power, the same company as Bel Fuse](https://www.belfuse.com/power-solutions)
- [PMBus: The Nervous System Bringing Digital Power to Life](https://www.digikey.com/en/articles/techzone/2016/oct/pmbus-the-nervous-system-bringing-digital-power-to-life) 2016 article at DigiKey

## Branches
- master - as of 2018 May, this is the current stable branch in use.
- SALT2v1_PowerDist2v0 (superceded) was the branch Bruce was working on but Scott was on master
- SALT2_PowerDist1 the old Master, supports SALT 2v0 and Power Dist 1v0, needs a special crossover cable between SALT and Power Dist with the swapped pins on the SALT end. SALT J1.2 -> PowerDist J2.1, SALT J1.3 -> PD J2.6, J1.1 -> J2.3, etc per schematics. This applies only to this revision of these boards.

## LCM300 Key Features
 - Active PFC
 - High MTBF
 - ball-bearing cooling fan, variable speed, very quiet even under full load
 - PMbus 1.1 at 100 kHz
 - 5V 2A unregulated output option. Also conformal coat option.
 - 3 year warranty
 - available from Sager Power Systems
 - 300 watts for about $100 in OEM quantities

## LCM300 PMBus Oddities
 - Some commands are not implemented by some devices. For example LCM300 does not implement COEFFICIENTS 0x30, rather they are published in the documentation.
 - ASCII data, at least in the LCM300, is stored in this format: {len} {asci char bytes} where the first byte returned by the command is the number of ascii
 chars to expect. Then the ascii chars follow. Then a garbage char if you keep reading, then 0xFF. No null terminator. This is actually pretty convenient. The
 length of ASCII data given in the LCM300 data sheet is almost always wrong. So we use the len value from the PMBus interface not the data sheet values.

## Release Information
- As of 2018 May all the important commands are working.

## Functions
- TODO add new functions
 - command_raw_read (int cmd, size_t count, char *data) useful mostly for debugging and exploration, it is how I discovered many things about the LCM300 data format. Read cmd for count bytes and store the data in char data[]. This lets you try to print out the data as a string as well as inspecting it individually or as chars. 
 - command_ascii_read (int cmd, size_t length, char *data, bool debug)

## Examples
- TODO add new examples
 - LCM300Q_CmdAsciiRead prints out a handful of ascii command values as proper length (read from the command response), null-terminated strings
 - LCM300Q_CmdRawRead
 - LCM300Q_V_I_Out_Read is a development test to verify "linear mode" data interpretation

### References
 - [PMBus 1.1 Spec in two parts](http://pmbus.org/) also see notes in cpp header file
 - PMBus 1.1 is based on [SMBus 1.1](smbus.org/specs/smbus110.pdf)
 
### Comments
 - Developed and tested with the LCM300Q 24V 300W supply
 - Note there is a serious error in the data sheet: the 5V standby supply is NOT regulated to 1% over load, it is unregulated and can vary from 5.4 to 4.8 volts no-load to full-load, which generally makes it unusable as a 5V logic supply. 5V devices typically require 5V +/- 5%. Devices which use the 5V supply as a voltage reference (not a good idea but they do exist) are especially susceptible to such a wide variation.
 - Also included is an example/test program 
 - See the source code for plenty of explanatory comments

###
#define I2C_BUS_ENABLE 4
	Using library Systronix_LCM300 in folder: C:\Users\BAB\Documents\code\Arduino\libraries\Systronix_LCM300 (legacy)
	Using library i2c_t3 in folder: C:\Users\BAB\Documents\code\Arduino\libraries\i2c_t3 (legacy)
	Sketch uses 46236 bytes (17%) of program storage space. Maximum is 262144 bytes.
	Global variables use 6772 bytes (10%) of dynamic memory, leaving 58764 bytes for local variables. Maximum is 65536 bytes.

#define I2C_BUS_ENABLE 2
Using library Systronix_LCM300 in folder: C:\Users\BAB\Documents\code\Arduino\libraries\Systronix_LCM300 (legacy)
Using library i2c_t3 in folder: C:\Users\BAB\Documents\code\Arduino\libraries\i2c_t3 (legacy)
Sketch uses 46236 bytes (17%) of program storage space. Maximum is 262144 bytes.
Global variables use 6772 bytes (10%) of dynamic memory, leaving 58764 bytes for local variables. Maximum is 65536 bytes.

### TODO
 - add doxygen docs

