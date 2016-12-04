# Systronix_LCM300
Arduino library for the Artesyn LCM300 family of 24/36/48V supplies with PMBus

## LCM300 Key Features
 - Active PFC
 - PMBus 1.1 at 100 KHz
 
### Comments
 - Developed and tested with the LCM300Q 24V 300W supply
 - Note there is a serious error in the data sheet: the 5V standby supply is NOT regulated to 1% over load, it is unregulated and can vary from 5.4 to 4.8 volts no-load to full-load, which generally makes it unusable as a 5V logic supply. 5V devices typically require 5V +/- 5%
 - Also included is an example/test program 
 - See the source code for plenty of explanatory comments

### TODO
 - add doxygen docs

