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
		tally_transaction (SILLY_PROGRAMMER);
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
	if (SUCCESS != command_raw_read (VOUT_MODE_CMD))	// did we get the raw exponent?
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


//---------------------------< T A L L Y _ T R A N S A C T I O N >--------------------------------------------
/**
Here we tally errors.  This does not answer the what-to-do-in-the-event-of-these-errors question; it just
counts them.

TODO: we should decide if the correct thing to do when slave does not ack, or arbitration is lost, or
timeout occurs, or auto reset fails (states 2, 5 and 4, 7 ??? state numbers may have changed since this
comment originally added) is to declare these addresses as non-existent.

We need to decide what to do when those conditions occur if we do not declare the device non-existent.
When a device is declared non-existent, what do we do then? (this last is more a question for the
application than this library).  The questions in this TODO apply equally to other i2c libraries that tally
these errors.

Don't set error.exists = false here! These errors are likely recoverable. bab & wsk 170612

This is the only place we set error.error_val()

TODO use i2c_t3 error or status enumeration here in the switch/case
*/

void Systronix_LCM300::tally_transaction (uint8_t value)
	{
	if (value && (error.total_error_count < UINT64_MAX))
		error.total_error_count++; 			// every time here incr total error count

	error.error_val = value;

	switch (value)
		{
		case SUCCESS:
			if (error.successful_count < UINT64_MAX)
				error.successful_count++;
			break;
		case 1:								// i2c_t3 and Wire: data too long from endTransmission() (rx/tx buffers are 259 bytes - slave addr + 2 cmd bytes + 256 data)
			error.data_len_error_count++;
			break;
#if defined I2C_T3_H
		case I2C_TIMEOUT:
			error.timeout_count++;			// 4 from i2c_t3; timeout from call to status() (read)
#else
		case 4:
			error.other_error_count++;		// i2c_t3 and Wire: from endTransmission() "other error"
#endif
			break;
		case 2:								// i2c_t3 and Wire: from endTransmission()
		case I2C_ADDR_NAK:					// 5 from i2c_t3
			error.rcv_addr_nack_count++;
			break;
		case 3:								// i2c_t3 and Wire: from endTransmission()
		case I2C_DATA_NAK:					// 6 from i2c_t3
			error.rcv_data_nack_count++;
			break;
		case I2C_ARB_LOST:					// 7 from i2c_t3; arbitration lost from call to status() (read)
			error.arbitration_lost_count++;
			break;
		case I2C_BUF_OVF:
			error.buffer_overflow_count++;
			break;
		case I2C_SLAVE_TX:
		case I2C_SLAVE_RX:
			error.other_error_count++;		// 9 & 10 from i2c_t3; these are not errors, I think
			break;
		case WR_INCOMPLETE:					// 11; Wire.write failed to write all of the data to tx_buffer
			error.incomplete_write_count++;
			break;
		case SILLY_PROGRAMMER:				// 12
			error.silly_programmer_error++;
			break;
		default:
			error.unknown_error_count++;
			break;
		}
	}


//---------------------------< C O M M A N D _ R A W _ R E A D >----------------------------------------------
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

uint8_t Systronix_LCM300::command_raw_read (int cmd_idx, bool debug)
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
		tally_transaction (WR_INCOMPLETE);						// increment the appropriate counter
		return FAIL;
		}

	_wire.endTransmission(I2C_NOSTOP); 							// don't send a stop condition, PMBus wants a repeated start

	if (debug) Serial.printf("cmd 0x%X, ", cmd[cmd_idx].cmd_byte);

	ret_val = _wire.requestFrom(_base, count, I2C_STOP);		// returns # of bytes received
	if (0 == ret_val || ASCII < ret_val)						// 0 is error; so is more than 17
		{
		Serial.printf ("raw read: invalid response length: %d bytes\n", ret_val);
		ret_val = _wire.status();								// to get error value
		tally_transaction (ret_val);							// increment the appropriate counter
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
