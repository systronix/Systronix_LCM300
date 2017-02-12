/******************************************************************************/
/*!
	@file		Systronix_LCM300Q.cpp
	
	@author		B Boyes (Systronix Inc)
    @license	TBD (see license.txt)	
    @section	HISTORY


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

/** -------- PMBus Support -------------------------------------------------------------------------

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

void Systronix_LCM300::setup(uint8_t base)
	{
	_base = base;
	BaseAddr = base;

//	struct data _data;			// instance of the data struct - not used
//	_data.address = _base;		// struct - not used
	}


//---------------------------< B E G I N >--------------------------------------------------------------------
/*!
	@brief  Join the I2C bus as a master at BaseAddr, 100 kHz clock

	TODO slave address must still be supplied with beginTransmission so what good does it do to specify it here?
*/

void Systronix_LCM300::begin(void)
	{
	Wire.begin(_base);	// join I2C as master
	}


//---------------------------< I N I T >----------------------------------------------------------------------
//
// Attempts to write the pointer register.  If successful, sets control.exists true, else false.
//

uint8_t Systronix_LCM300::init (uint16_t config)
	{
	uint8_t written;
	
	Wire.beginTransmission (_base);						// base address

  	if (Wire.endTransmission())
		{
		control.exists = false;							// unsuccessful i2c transaction
		return FAIL;
		}
	
	control.exists = true;								// if here, we appear to have communicated with
	return SUCCESS;										// the tmp102
	}


//---------------------------< T A L L Y _ E R R O R S >------------------------------------------------------
//
// Here we tally errors.  This does not answer the 'what to do in the event of these errors' question; it just
// counts them.  If the device does not ack the address portion of a transaction or if we get a timeout error,
// exists is set to false.  We assume here that the timeout error is really an indication that the automatic
// reset feature of the i2c_t3 library failed to reset the device in which case, the device no longer 'exists'
// for whatever reason.
//

void Systronix_LCM300::tally_errors (uint8_t error)
	{
	switch (error)
		{
		case 0:					// Wire.write failed to write all of the data to tx_buffer
			control.incomplete_write_count ++;
			break;
		case 1:					// data too long from endTransmission() (rx/tx buffers are 259 bytes - slave addr + 2 cmd bytes + 256 data)
		case 8:					// buffer overflow from call to status() (read - transaction never started)
			control.data_len_error_count ++;
			break;
		case 2:					// slave did not ack address (write)
		case 5:					// from call to status() (read)
			control.rcv_addr_nack_count ++;
			control.exists = false;
			break;
		case 3:					// slave did not ack data (write)
		case 6:					// from call to status() (read)
			control.rcv_data_nack_count ++;
			break;
		case 4:					// arbitration lost (write) or timeout (read/write) or auto-reset failed
		case 7:					// arbitration lost from call to status() (read)
			control.other_error_count ++;
			control.exists=false;
		}
	}







//---------------------------< W R I T E R E G I S T E R >----------------------------------------------------
/**
Param pointer is the LCM300Q register into which to write the data
data is the 16 bits to write.
returns 0 if no error, positive values for NAK errors
**/

uint8_t Systronix_LCM300::writeRegister (uint8_t pointer, uint16_t data)
	{
	uint8_t written;									// number of bytes written

	if (!control.exists)								// exit immediately if device does not exist
		return ABSENT;

	Wire.beginTransmission (_base);						// base address
	written = Wire.write (pointer);						// pointer in 2 lsb
	written += Wire.write ((uint8_t)(data >> 8));		// write MSB of data
	written += Wire.write ((uint8_t)(data & 0x00FF));	// write LSB of data

	if (3 != written)
		{
		control.ret_val = 0;
		tally_errors (control.ret_val);					// increment the appropriate counter
		return FAIL;
		}
	
  	if (SUCCESS == Wire.endTransmission())
		return SUCCESS;
	tally_errors (control.ret_val);						// increment the appropriate counter
	return FAIL;										// calling function decides what to do with the error
	}


//---------------------------< R E A D   C O M M A N D >--------------------------------------------
/**
  Read the 16-bit register addressed by the command, store the data at the location passed
  
  return 0 if no error, positive bytes read otherwise.
*/

uint8_t Systronix_LCM300::readRegister (uint16_t *data)
	{
	if (!control.exists)								// exit immediately if device does not exist
		return ABSENT;

	if (2 != Wire.requestFrom(_base, 2, I2C_STOP))
		{
		control.ret_val = Wire.status();				// to get error value
		tally_errors (control.ret_val);					// increment the appropriate counter
		return FAIL;
		}

	*data = (uint16_t)Wire.read() << 8;
	*data |= (uint16_t)Wire.read();
	return SUCCESS;
	}

//---------------------------< COMMAND RAW READ >------------------------------------------------------
/**
Read the RAW data received in response to cmd, store it in char array data
Data could be any type, so attempt to print as string may be unreadable.
But it is useful for debug and exploration

Note that PMBus reads are not memory locations, and whatever number you request seems to be "available"
even if the data is really junk. For example you can "read" 'm' number of bytes from a command which only 
returns 'n' actual data bytes. Usually n+1 is some consistent value <0xFF and n+[2,3,...] are 0xFF

Note that byte 0 of LCM300 ascii commands is the length of the string, and string is not null terminated.

@TODO implement ABSENT check

@return SUCCESS, FAIL, or ABSENT
*/

uint8_t Systronix_LCM300::commandRawRead (int cmd, size_t count, char *data)
	{
	// if (!control.exists)								// exit immediately if device does not exist
	// {
	// 	Serial.println("No control");
	// 	return ABSENT;
	// }

	uint8_t written;	// # of bytes written 

	Wire.beginTransmission (_base);						// base address
	written = Wire.write (cmd);							// PMBus command code
	Wire.endTransmission(I2C_NOSTOP); 					// don't send a stop condition, PMBus wants a repeated start

	Serial.printf("cmd 0x%X, ", cmd);

	char char_read;
	// now try to read the ascii data at that read command location

	if (count != Wire.requestFrom(_base, count, I2C_STOP))
		{
		Serial.println("raw read wrong number of bytes available");
		control.ret_val = Wire.status();				// to get error value
		tally_errors (control.ret_val);					// increment the appropriate counter
		return FAIL;
		}

	Serial.printf(" read %i bytes\r\n", count);	


	uint8_t index=0;
	while (Wire.available())
		{
		char_read = Wire.read();
		Serial.printf("%u:0x%02X/%c ", index, char_read, char_read);
		data[index++] = char_read;
		}

	Serial.println();
	return SUCCESS;
	}

//---------------------------< COMMAND ASCII READ >------------------------------------------------------
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

@TODO implement ABSENT check

@return SUCCESS, FAIL, or ABSENT
*/

uint8_t Systronix_LCM300::commandAsciiRead (int cmd, size_t length, char *data)
	{
	// if (!control.exists)								// exit immediately if device does not exist
	// {
	// 	Serial.println("No control");
	// 	return ABSENT;
	// }

	uint8_t written;	// # of bytes written 
	size_t count;		// # bytes to read, generally length + 2

	Wire.beginTransmission (_base);						// base address
	written = Wire.write (cmd);							// PMBus command code
	Wire.endTransmission(I2C_NOSTOP); 					// don't send a stop condition, PMBus wants a repeated start

	Serial.printf("ascii read, cmd: 0x%X\r\n", cmd);

	char char_read;
	// now try to read the ascii data at that read command location

	// 0th byte is the length of the ascii data, always >=2 and < 16 for LCM300
	// add length byte and one more for good measure
	count = 1;
	if (count != Wire.requestFrom(_base, count, I2C_STOP))
		{
		Serial.printf("ascii read of 1st byte failed\r\n");
		data[0] = 0;	// null term
		return FAIL;
		}
	else
		{
		// should read length of ascii data avail at this command
		char_read = Wire.read();
		Serial.printf("ascii length byte = %u\r\n", (uint8_t) char_read);
		}

	Wire.beginTransmission (_base);						// base address
	written = Wire.write (cmd);							// PMBus command code
	Wire.endTransmission(I2C_NOSTOP); 					// don't send a stop condition, PMBus wants a repeated start

	// now read the ascii data based on length we already read, but we re-read the length byte
	count = char_read + 1;	// length of ascii data
	if (count != Wire.requestFrom(_base, count, I2C_STOP))
		{
		Serial.printf("ascii data chars failed");
		control.ret_val = Wire.status();				// to get error value
		tally_errors (control.ret_val);					// increment the appropriate counter
		return FAIL;
		}
	else
		{
		Serial.printf("%u ascii chars read\r\n", (uint8_t)count);	
		}

	uint8_t index=0;
	while (Wire.available())
		{
		char_read = Wire.read();
		Serial.printf("%u:0x%02X/%c ", index, char_read, char_read);
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

	if (index > 2) data[index-1] = 0x0; 	// null term

	Serial.println();
	return SUCCESS;
	}
