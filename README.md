# Systronix_LCM300
Arduino library for the Artesyn LCM300 family of 24/36/48V supplies with PMBus

## LCM300 Key Features
 - Active PFC
 - High MTBF
 - ball-bearing cooling fan, variable speed, very quiet even under full load
 - PMbus 1.1 at 100 kHz
 - 5V 2A unregulated output option. Also conformal coat option.
 - 3 year warranty
 - available from Sager Power Systems
 - 300 watts for about $100 in OEM quantities

## PMBus Oddities
 - Some commands are not implemented by some devices. For example LCM300 does not implement COEFFICIENTS 0x30, rather they are published in the documentation.
 - ASCII data, at least in the LCM300, is stored in this format: {len} {asci char bytes} where the first byte returned by the command is the number of ascii
 chars to expect. Then the ascii chars follow. Then a garbage char if you keep reading, then 0xFF. No null terminator. This is actually pretty convenient. The
 length of ASCII data given in the LCM300 data sheet is almost always wrong. So we use the len value from the PMBus interface not the data sheet values.

### References
 - [PMBus 1.1 Spec in two parts](http://pmbus.org/) also see notes in cpp header file
 - PMBus 1.1 is based on [SMBus 1.1](smbus.org/specs/smbus110.pdf)
 
### Comments
 - Developed and tested with the LCM300Q 24V 300W supply
 - Note there is a serious error in the data sheet: the 5V standby supply is NOT regulated to 1% over load, it is unregulated and can vary from 5.4 to 4.8 volts no-load to full-load, which generally makes it unusable as a 5V logic supply. 5V devices typically require 5V +/- 5%. Devices which use the 5V supply as a voltage reference (not a good idea but they do exist) are especially susceptible to such a wide variation.
 - Also included is an example/test program 
 - See the source code for plenty of explanatory comments

### TODO
 - add doxygen docs

