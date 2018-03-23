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

-------- Teensy Target Hardware and i2c_t3 library

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

-------- PMBus Support -------------------------------------------------------------------------

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

--------  PMBus Addressing --------

The address bits A2, A1, A0 are internally pulled up to 2.7V so by default they are asserted.
LCM300 base address is 8-bit address 0xBE including R/W bit as 0 (B 1011 111x) where x is R/W
and the 3 lsb A[2..0] are in default pulled-up states

By the Arduino convention 8-bit 0xBE is 7-bit address 0x5F ignoring the R/W bit
If all the address bits are low, that is 7-bit address 0x58 ignoring the R/W bit.
Therefore the possible address range is the eight 7-bit addresses 0x5[8..F]

--------  PMBus Reads --------



--------  PMBus Writes --------

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
// Attempts to write the pointer register.  If successful, sets control.exists true, else false.
//

uint8_t Systronix_LCM300::init (void)
	{
	_wire.beginTransmission (_base);					// base address

	if (_wire.endTransmission())						// returns 0 if got an ACK from LCM300
		{
		error.exists = false;							// unsuccessful i2c transaction
		return FAIL;
		}
	
	error.exists = true;								// if here, we appear to have communicated with
	return SUCCESS;										// the LCM300
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


//---------------------------< R E G I S T E R _ W R I T E >--------------------------------------------------
/**
Param pointer is the LCM300Q register into which to write the data
data is the 16 bits to write.
returns SUCCESS or FAIL
**/

uint8_t Systronix_LCM300::register_write (uint8_t pointer, uint16_t data)
	{
	uint8_t ret_val;

	if (!error.exists)									// exit immediately if device does not exist
		return ABSENT;

	_wire.beginTransmission (_base);					// base address
	ret_val = _wire.write (pointer);					// pointer in 2 lsb
	ret_val += _wire.write ((uint8_t)(data >> 8));		// write MSB of data
	ret_val += _wire.write ((uint8_t)(data & 0x00FF));	// write LSB of data

	if (3 != ret_val)
		{
		tally_transaction (WR_INCOMPLETE);				// increment the appropriate counter
		return FAIL;
		}

	ret_val = _wire.endTransmission();
	
	if (SUCCESS != ret_val)
		{
		tally_transaction (ret_val);					// increment the appropriate counter
		return FAIL;									// calling function decides what to do with the error
		}

	tally_transaction (SUCCESS);
	return SUCCESS;
	}


//---------------------------< R E G I S T E R _ R E A D >-----------------------------------------------------
//
// Read the 16-bit register addressed by the command, store the data at the location passed
//
// returns SUCCESS or FAIL
//

uint8_t Systronix_LCM300::register_read (uint16_t *data)
	{
	uint8_t ret_val;

	if (!error.exists)								// exit immediately if device does not exist
		return ABSENT;

	if (2 != _wire.requestFrom(_base, 2, I2C_STOP))
		{
		ret_val = _wire.status();					// to get error value
		tally_transaction (ret_val);				// increment the appropriate counter
		return FAIL;
		}

	*data = (uint16_t)_wire.readByte() << 8;
	*data |= (uint16_t)_wire.readByte();

	tally_transaction (SUCCESS);
	return SUCCESS;
	}


//---------------------------< C O M M A N D _ R A W _ R E A D >----------------------------------------------
/**
Read the RAW data received in response to cmd, store it in char array data
Data could be any type, so an attempt to print as string may be unreadable.
But it is useful for debug and exploration

Note that PMBus reads are not memory locations, and whatever number you request seems to be "available"
even if the data is really junk. For example you can "read" 'm' number of bytes from a command which only 
returns 'n' actual data bytes. Usually n+1 is some consistent value <0xFF and n+[2,3,...] are 0xFF

Note that byte 0 of LCM300 ascii commands is the length of the string, and string is not null terminated.

@return SUCCESS, FAIL, or ABSENT
*/

uint8_t Systronix_LCM300::command_raw_read (int cmd, size_t count, char *data)
	{
	uint8_t ret_val;
	char char_read;

	if (!error.exists)								// exit immediately if device does not exist
		return ABSENT;

	_wire.beginTransmission (_base);						// base address
	ret_val = _wire.write (cmd);							// PMBus command code

	if (1 != ret_val)
		{
		tally_transaction (WR_INCOMPLETE);				// increment the appropriate counter
		return FAIL;
		}

	_wire.endTransmission(I2C_NOSTOP); 					// don't send a stop condition, PMBus wants a repeated start

	Serial.printf("cmd 0x%X, ", cmd);

	// now try to read the ascii data at that read command location

	if (count != _wire.requestFrom(_base, count, I2C_STOP))
		{
		Serial.println("raw read wrong number of bytes available");
		ret_val = _wire.status();						// to get error value
		tally_transaction (ret_val);					// increment the appropriate counter
		return FAIL;
		}

	Serial.printf(" read %i bytes\r\n", count);	

	uint8_t index=0;
	while (_wire.available())
		{
		char_read = _wire.readByte();
		Serial.printf("%u:0x%02X/%c ", index, char_read, char_read);
		data[index++] = char_read;
		}

	Serial.printf ("\n");
	return SUCCESS;
	}


//---------------------------< C O M M A N D _ A S C I I _ R E A D >------------------------------------------
/**
Read at most length chars of ascii data received in response to cmd, store it in char array data
The first byte of ascii data on the LCM300 is the length which value is always >=2 and < 16, if not, 
make 0th byte of array null and return FAIL.
Of course the first byte could actually be the LSB of a linear number, so we check further.
If first byte < length, try to read that many ascii bytes. If each is not ascii range, make 0th byte of array null
and return FAIL.
If data appears to be ASCII, copy data to char array and null terminate.

The PMBus wants you to send the command, then a single restart to read the data. It doesn't like multiple restarts
to read the data piecemeal. This means you can't read the length byte and then read the data in separate read cycles.
To read just the length it would be necessary to then start another command cycle. Which might be an OK approach.
Then this function could read only the exact ascii data available.

@param cmd, use the defined constants in the header file to be safe.
@param length the most we will try to read. Maybe not needed now that we read ascii length. 
However if this is not really ascii data we could misread some large value as the length.
So this also prevents buffer overrun
@param *data pointer to array to put the ascii data. Length value stripped. Null terminated.
@param debug print out detailed cmd value, length byte, # read, hex values + char if printable 
@return SUCCESS, FAIL, or ABSENT
*/

uint8_t Systronix_LCM300::command_ascii_read (int cmd, size_t length, char *data, bool debug)
	{
	uint8_t ret_val;
	size_t count;							// # bytes to read, generally length + 2
	char char_read;
	char char_print = 0x20;

	if (!error.exists)						// exit immediately if device does not exist
		return ABSENT;


	_wire.beginTransmission (_base);						// base address
	ret_val = _wire.write (cmd);							// PMBus command code

	if (1 != ret_val)
		{
		tally_transaction (WR_INCOMPLETE);				// increment the appropriate counter
		return FAIL;
		}

	_wire.endTransmission(I2C_NOSTOP); 					// don't send a stop condition, PMBus wants a repeated start

	if (debug) Serial.printf("ascii read, cmd: 0x%X\r\n", cmd);

	// now try to read the ascii data at that read command location

	// 0th byte is the length of the ascii data, always >=2 and < 16 for LCM300
	// add length byte and one more for good measure
	count = 1;
	if (count != _wire.requestFrom(_base, count, I2C_STOP))
		{
		Serial.printf("ascii read of 1st byte failed\r\n");
		data[0] = '\0';	// null term
		return FAIL;
		}
	else
		{
		// should read length of ascii data avail at this command
		char_read = _wire.readByte();
		if (debug) Serial.printf("ascii length byte = %u\r\n", (uint8_t) char_read);
		}

	_wire.beginTransmission (_base);						// base address
	ret_val = _wire.write (cmd);							// PMBus command code

	if (1 != ret_val)
		{
		tally_transaction (WR_INCOMPLETE);				// increment the appropriate counter
		return FAIL;
		}

	_wire.endTransmission(I2C_NOSTOP); 					// don't send a stop condition, PMBus wants a repeated start

	// now read the ascii data based on length we already read, but we re-read the length byte
	count = char_read + 1;	// length of ascii data including length byte
	if (count != _wire.requestFrom(_base, count, I2C_STOP))
		{
		Serial.printf("ascii data chars failed");
		ret_val = _wire.status();				// to get error value
		tally_transaction (ret_val);					// increment the appropriate counter
		return FAIL;
		}
	else
		{
		if (debug) Serial.printf("%u ascii chars read\r\n", (uint8_t)count);	
		}

	uint8_t index=0;
	while (_wire.available())
		{
		char_read = _wire.read();
		if ((char_read < 0x20) || (char_read > 0x7E)) 
			{
			char_print = 0x20;	// unprintable? Use space
			}
		else
			{
			char_print = char_read;	// printable ascii	
			}
		if (debug) Serial.printf("%u:0x%02X/%c ", index, char_read, char_print);
		if (0 == index)
			{
				// discard length in byte 0
			}
		else 
			{
				data[index-1] = char_read;
			}
		index++;
		}

	if (index > 2)
		data[index-1] = '\0'; 	// null term

	if (debug) Serial.println();
	return SUCCESS;
	}
