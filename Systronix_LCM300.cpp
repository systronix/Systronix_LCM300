/******************************************************************************/
/*!
	@file		Systronix_LCM300Q.cpp MASTER

	@author		B Boyes (Systronix Inc)
    @license	TBD (see license.txt)
    @section	HISTORY



	v0.2	2018Mar22 bboyes finishing up thanks to some tech support from Artesyn
    v0.1	2016Dec01 bboyes Start based on LCM300Q library

*/
/******************************************************************************/

/** --------  Description --------------------------------------------------------------------------

This is a driver for the Artesyn LCM300 Series 300 Watt AC-to-DC power supplies, in particular the
LCM300Q 24 volt output version.

Caution: the 5V 2A standby power available as option '4' is unregulated! 
It can vary from 4.8 - 5.4 volts. Confirmed by email 2016 Nov 23.
The data sheet claiming 1% regulation is wrong. We notified Artesyn of this.

-------- Teensy Target Hardware and i2c_t3 library --------

i2c_t3 documentation is at https://forum.pjrc.com/threads/21680-New-I2C-library-for-Teensy3

Note not all Teensy 3.X have the same I2C hardware!
Teensy 3.0 has only one I2C called Wire, which can use pins 18 & 19 or 16 & 17
Teensy 3.1, 3.2, LC have Wire and Wire1 which can exist on a large number of pins



-------- Technical Information Sources --------

See the Artesyn LCM300 family data sheet at 
https://www.artesyn.com/power/power-supplies/websheet/491/lcm300-series
This library is written using these sources of information:
Artesyn Data Sheet LCM300 310 Watt Bulk Front End, Rev 10.28.14 (last page, bottom right)
Sager LCM300 Series 300 Watt Covered with Fan Power Supply, no revision but with Sager logo and 
web links.
Artesyn Technical Reference Note Rev.05.03.16_#1.2 (all PMBus details)
Phone and e-mail communication with Artesyn technical support

The LCM300 series includes 24V output, the LCM300Q. LCM300U is 36V, LCM300W is 48V.

-------- PMBus Support --------

All LCM300 series support PMBus Version 1.1 at up to 100 KHz. No 400 kHz possible. This is
according to email from Artesyn tech support. Command 0x98 holds "22" as PMBus revision; what
can this really mean?

Get the PMBus spec free here: http://pmbus.org/Specifications/OlderSpecifications

All LCM300 series have some commands in common. Other commands differ slightly
depending on the version due to different output voltage ranges. For example
command 21h, VOUT_COMMAND has a different default value and range for each
supply in the family. For the LCM300Q, 21h default is 0x2FE6 = 24V and the
range is 19.09 to 33.60 volts. However VOUT_MAX (command 24h) limits max output
voltage to 0x3999 = 28.9V

-------- PMBus Addressing --------

The address bits A2, A1, A0 are internally pulled up to 2.7V so by default they are asserted.
LCM300 base address is 8-bit address 0xBE including R/W bit as 0 (B 1011 111x) where x is R/W
and the 3 lsb A[2..0] are in default pulled-up states

By the Arduino convention 8-bit 0xBE is 7-bit address 0x5F ignoring the R/W bit
If all the address bits are low, that is 7-bit address 0x58 ignoring the R/W bit.
Therefore the possible address range is the eight 7-bit addresses 0x5[8..F]

-------- PMBus Reads --------



-------- PMBus Writes --------

Page 35 of the Technical Note calls out the need to enable writing to writeable registers before
you can change their contents.

--------------------------------------------------------------------------------------------------*/

#include <Systronix_LCM300.h>


//---------------------------< S E T U P >--------------------------------------------------------------------
/*!
	@brief  Instantiates a new LCM300Q class to use the given base address
	@todo	Test base address for legal range 0x58 - 0x5F
			Add constructor(void) for default address of 0x5F
*/

uint8_t Systronix_LCM300::setup (uint8_t base, i2c_t3 wire, char* name)
	{
	if ((LCM300_BASE_MIN > base) || (LCM300_BASE_MAX < base))
		{
		i2c_common.tally_transaction (SILLY_PROGRAMMER, &error);
		return FAIL;
		}

	_base = base;
	_wire = wire;
	_wire_name = wire_name = name;		// protected and public
	return SUCCESS;
	}


//---------------------------< B E G I N >--------------------------------------------------------------------
/*!
	@brief  Join the I2C bus as a master at BaseAddr, 100 kHz clock
*/

void Systronix_LCM300::begin (i2c_pins pins)
	{
	_wire.begin(I2C_MASTER, 0x00, pins, I2C_PULLUP_EXT, I2C_RATE_100);	// join I2C as master
//	Serial.printf("LCM300 lib begin %s\r\n", _wire_name);
	_wire.setDefaultTimeout(200000); // 200ms
	}


//---------------------------< B A S E _ G E T >--------------------------------------------------------------
//
// return the I2C base address for this instance
//

uint8_t Systronix_LCM300::base_get(void)
	{
	return _base;
	}


//---------------------------< I N I T >----------------------------------------------------------------------
//
// Attempts to fetch the Vout Mode byte (voltage measurement exponent).  If successful, sets control.exists true, false else.
//
// Exponent is in the 5 lsbs; signed 2's complement; upper three bits may have value but for the exponent should
// be set or cleared according to the state of bit 4 (sign bit); this is the sign extension.
//

uint8_t Systronix_LCM300::init (void)
	{
	error.exists = true;								// here, assume that the device exists
	if (SUCCESS != command_read (VOUT_MODE_CMD))		// did we get the raw exponent?
		{
		error.exists = false;							// unsuccessful i2c transaction
		return ABSENT;
		}
														// sign extend the exponent to 8 bits
	_linear_exponent = (cmd_response.as_byte & 0x10) ? (cmd_response.as_byte | 0xE0) : (cmd_response.as_byte & 0x1F);
	_vout_mode = (cmd_response.as_byte & 0xE0) >> 5;	// shift mode bits into 3 lsbs

	return SUCCESS;
	}


//---------------------------< R E S E T _ B U S >------------------------------------------------------------
/**
	Invoke resetBus of whichever Wire net this class instance is using
	@return nothing
*/
void Systronix_LCM300::reset_bus (void)
	{
	_wire.resetBus();
	}


//---------------------------< R E S E T _ B U S _ C O U N T _ R E A D >--------------------------------------
/**
	Return the resetBusCount of whichever Wire net this class instance is using
	@return number of Wire net resets, clips at UINT32_MAX
*/
uint32_t Systronix_LCM300::reset_bus_count_read(void)
	{
	return _wire.resetBusCountRead();
	}


//---------------------------< C L E A R _ F A U L T S _ C M D >----------------------------------------------
//
// 
//

uint8_t Systronix_LCM300::clear_faults_cmd (void)
	{
	uint8_t ret_val;

	if (!error.exists)										// exit immediately if device does not exist
		return ABSENT;

	delay(50);												// exists; ensure that we meet datasheet communication interval spec

	_wire.beginTransmission(_base);							// init tx buff for xmit to slave at _base address
	ret_val = _wire.write (LCM300_CLEAR_FAULTS_CMD);		// add command byte to the tx buffer
	if (1 != ret_val)
		{
		i2c_common.tally_transaction (WR_INCOMPLETE, &error);					// only here 0 is error value since we expected to write more than 0 bytes
		return FAIL;
		}

	ret_val = _wire.endTransmission();						// xmit command byte
	if (SUCCESS != ret_val)
		{
		i2c_common.tally_transaction (ret_val, &error);						// increment the appropriate counter
		return FAIL;										// calling function decides what to do with the error
		}

	i2c_common.tally_transaction (SUCCESS, &error);
	return SUCCESS;
	}


//---------------------------< C O M M A N D _ R E A D >------------------------------------------------------
/**
Read the RAW data received in response to cmd, store it in cmd_response.as_array[]
Data could be any type, so an attempt to print as string may be unreadable.
But it is useful for debug and exploration
Note that PMBus reads are not memory locations, and whatever number you request seems to be "available"
even if the data is really junk. For example you can "read" 'm' number of bytes from a command which only 
returns 'n' actual data bytes. Usually n+1 is some consistent value <0xFF and n+[2,3,...] are 0xFF
Note that byte 0 of LCM300 ascii commands is the length of the string, and string is not null terminated.
@return SUCCESS, FAIL, or ABSENT
*/

uint8_t Systronix_LCM300::command_read (int cmd_idx, bool debug)
	{
	uint8_t ret_val;
	size_t	count = cmd[cmd_idx].count;							// number of bytes to read
	uint8_t index = 0;

	if (!error.exists)											// exit immediately if device does not exist
		return ABSENT;

	delay(50);													// exists; ensure that we meet datasheet communication interval spec

	_wire.beginTransmission (_base);							// base address
	ret_val = _wire.write (cmd[cmd_idx].cmd_byte);				// PMBus command code

	if (1 != ret_val)
		{
		i2c_common.tally_transaction (WR_INCOMPLETE, &error);						// increment the appropriate counter
		return FAIL;
		}

	_wire.endTransmission(I2C_NOSTOP); 							// don't send a stop condition, PMBus wants a repeated start

	if (debug) Serial.printf("cmd 0x%X, ", cmd[cmd_idx].cmd_byte);

	ret_val = _wire.requestFrom(_base, count, I2C_STOP);		// returns # of bytes received
	if (0 == ret_val || ASCII < ret_val)						// 0 is error; so is more than 17
		{
		Serial.printf ("raw read: invalid response length: %d bytes\n", ret_val);
		ret_val = _wire.status();								// to get error value
		i2c_common.tally_transaction (ret_val, &error);							// increment the appropriate counter
		return FAIL;
		}

	if (debug) Serial.printf(" read %i bytes\r\n", ret_val);

	while (_wire.available())
		{
		cmd_response.as_array[index] = _wire.readByte();
		if (debug) Serial.printf("%u:0x%02X ", index, cmd_response.as_array[index]);
		index++;
		}

	if (ASCII == count)											// an ascii response so null terminate it
		{														// as_array[0] holds length of remaining response in bytes
		index = cmd_response.as_array[0] + 1;					// <length>+1 is index of null terminator
		cmd_response.as_array[index] = '\0';					// null terminate
		}

	if (debug) Serial.printf ("\n");

	i2c_common.tally_transaction (SUCCESS, &error);
	return SUCCESS;
	}


//---------------------------< R A W _ V O L T A G E _ T O _ F L O A T >--------------------------------------
//
// Voltage measurements appear to be rendered in a different form from all other linear measurements reported
// by the LCM300.  These measurements use a separate exponent read from the LCM300 with the VOUT_MODE_CMD (0x20).
//
// _linear_exponent is set by init() and is simply a sign-extended version of the raw value returned by the
// VOUT_MODE_CMD (0x20)
//
// result = mantissa * 2.0^exponent
//
// This function assumes that _linear_exponent has been set to a valid value before this function is called.
// should not normally be an issue because _linear_exponent is set by init().
//

float Systronix_LCM300::raw_voltage_to_float (uint16_t volt_raw)
	{
	return (float)volt_raw * powf (2.0, (float)_linear_exponent);
	}


//---------------------------< P M B U S _ L I T E R A L _ T O _ F L O A T >----------------------------------
//
// PMbus literal values are 16-bit values where:
//		[15..11]	signed 2's complement exponent
//		[10..0]		signed 2's complement mantissa
// to calculate float value, extract exponent, sign extend so that exponent lsb from literal is now in bit 0.
// sign extend the mantissa
// result = mantissa * 2.0^exponent
//

float Systronix_LCM300::pmbus_literal_to_float (uint16_t literal_raw)
	{
	int16_t		exponent = (int16_t)(literal_raw & 0xF800) >> 11;							// get the exponent; int16_t cast required because 0xF800 is uint16_t
	int16_t		mantissa = literal_raw;														// get the mantissa
				mantissa = (mantissa & 0x0400) ? mantissa | 0xF800 : mantissa & 0x07FF;		// sign extend the mantissa

	return (float)mantissa * powf (2.0, (float)exponent);
	}


//---------------------------< P M B U S _ A V E R A G E _ P O W E R >----------------------------------------
//
// Calculate average power.  From PMBus Power System Mgt Protocol Specification 1.2 part II section 10.13 page 87,
// this calculation:
//		average_power = (energy_count - last_energy _count) / (sample_count - last_sample_count)
//
// where energy_count is the result from this equation:
//
//	energy_count = rollover_count * maximum_direct_format_value + accumulator_value
//		where:
//			coefficients: m=1, b=0, R=0
//			Ymax = 2^15-1 = 32767
//			maximum_direct_format_value = (mYmax + b) * 10^R = (1Ymax + 0) * 10^R = (1 * 32767 + 0) * 10^0 = 32767
//
// this function does all maintenance of the eout_data struct.
//
// this function must be called directly after a call to command_read (READ_EOUT_CMD)
//
// Accumulator:  the accumulator value is believed to be in PMBus direct format event though the 1v5 data sheet
// indicates that the READ_EOUT command returns 2 bytes in linear format.  Treating accumulator as if it is a
// PMBus linear value produces irrational results (often negative power) which indicates that accumulator cannot
// be PMBus linear.  The max value for accumulator in PMBus direct format is 32767 given the coefficients stated
// in the data sheet m=1, b=0, R=0.
//
// When accumulator overflows, rollover_counter is incremented (this too may rollover at rc = 255).  Because
// rollover_counter multiplies maximum_direct_format_value when calculating energy_count, when rollover_count
// rolls over to zero it is different from the rollover_count used to make last_energy_count, so average_power
// will be obviously wrong.  To get round that, when rollover_count is less than last_rollover_count, we modify
// accumulator by adding 32768 and use last_rollover_count to calculate the current energy_count.  These 'spoofing'
// values are not retained beyond the current energy_count calculation.
//

void Systronix_LCM300::pmbus_average_power (void)
	{
	uint32_t	iaccumulator;											// intermediate values that will be used in the calculations
	uint32_t	irollover_count;										// uint32 because intermediate results can be 25 bits
	uint32_t	isample_count;
	uint32_t	correction;												// used when rollover counter rolls over to 0

	eout_data.accumulator = *(uint16_t*)&cmd_response.as_array[1];		// get newly read raw data from command response union
	eout_data.rollover_count = cmd_response.as_array[3];
	eout_data.sample_count = *(uint32_t*)&cmd_response.as_array[4] & 0x00FFFFFF;	// 24 bits of sample count may have PEC so mask that off

	if (eout_data.rollover_count < eout_data.last_rollover_count)		// accumulator overflow occurs at 32767; handle the case when accumulator overflow
		{																// causes rollover_counter to rollover to 0.  When this happens we create spoof
		irollover_count = (uint32_t)eout_data.last_rollover_count;		// values for energy_count calculation by using last time's rollover_count.
																		// Because accumulator may have rolled over multiple times since last reading
																		// which may have caused rollover counter to rollover, calculate an accumulator
		correction = ((eout_data.rollover_count + 256) - eout_data.last_rollover_count) * 32768;	// correction to be added to accumulator.  Only valid
		iaccumulator = (uint32_t)eout_data.accumulator + correction;	// a single rollover counter rollover
		}
	else
		{																// no accumulator overflow
		irollover_count = (uint32_t)eout_data.rollover_count;			// so no spoofing required
		iaccumulator = (uint32_t)eout_data.accumulator;
		}
																		// when sample_count rolls over spoof the calculation by showing the rollover in isample_count
																		// rollover occurs at 16777215 (0x00FFFFFF)
	isample_count = (eout_data.sample_count < eout_data.last_sample_count) ? eout_data.last_sample_count + 16777216 : eout_data.sample_count;	// 0x01000000 = 2^24

	eout_data.energy_count = irollover_count * 32767 + iaccumulator;	// calculate new energy count
	eout_data.average_power = (eout_data.energy_count - eout_data.last_energy_count) / (isample_count - eout_data.last_sample_count);	// calculate average power

	if (eout_data.rollover_count < eout_data.last_rollover_count)		// when rollover_counter rolls over, calculate new last energy count using rolled over values
		eout_data.energy_count = eout_data.rollover_count * 32767 + eout_data.accumulator;	// from the most recent READ_EOUT command

	eout_data.last_energy_count = eout_data.energy_count;				// and save new 'lasts'
	eout_data.last_sample_count = eout_data.sample_count;				// always this even when it has rolled over
	eout_data.last_rollover_count = eout_data.rollover_count;
	}
