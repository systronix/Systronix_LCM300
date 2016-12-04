#ifndef SYSTRONIX_LCM300_h
#define SYSTRONIX_LCM300_h


/******************************************************************************/
/*!
	@file		Systronix_LCM300.h
	
	@author		B Boyes (Systronix Inc)
    @license	TBD (see license.txt)	
    @section	HISTORY


    v0.1	2016Dec01 bboyes Start based on LCM300 library

*/
/******************************************************************************/

/** TODO LIST

**/

/** --------  Description ------------------------------------------------------

See the Artesyn LCM300 family data sheet at 
https://www.artesyn.com/power/power-supplies/websheet/491/lcm300-series
This library is written using data sheet Rev.05.03.16_#1.2l

The LCM300 series includes 24V output, the LCM300Q. LCM300U is 36V, LCM300W is 48V.

All support PMBus Version 1.1 at up to 100 KHz
Get the PMBus spec free here: http://pmbus.org/Specifications/OlderSpecifications
All CLM300 series have some commands in common. Other commands differ slightly
depending on the version due to different output voltage ranges. For example
command 21h, VOUT_COMMAND has a different default value and range for each
supply in the family. For the LCM300Q, 21h default is 0x2FE6 = 24V and the
range is 19.09 to 33.60 volts. However VOUT_MAX (command 24h) limits max output
voltage to 0x3999 = 28.9V


------------------------------------------------------------------------------*/

/** --------  Device Addressing --------
LCM300 base address is 8-bit address 0xBE including R/W bit as 0 (B 1011 111x) where x is R/W
Or by the Arduino convetion that is 7-bit address 0x5F ignoring the R/W bit

The address bits A2, A1, A0 are internally pulled up to 2.7V so by default they are asserted
-------------------------------------*/

#include <Arduino.h>

#if defined (__MK20DX256__) || defined (__MK20DX128__) 	// Teensy 3.1 or 3.2 || Teensy 3.0
#include <i2c_t3.h>		
#else
#include <Wire.h>	// for AVR I2C library
#endif

#define		SUCCESS	0
#define		FAIL	0xFF
#define		ABSENT	0xFD

/** --------  Register Addresses --------
The two lsb of the pointer register hold the register bits, which
are used to address one of the four directly-accessible registers.

There are four pointer addresses:
00  Temperature Register (Read Only) 12-13 bits in ms position
	bit 0 in LS byte is 1 if in 13-bit 'extended mode' (EM)
	If temp is positive (msb=0) the value is just the binary value.
	If temp is negative (msb=0) the value is in 2's complement form,
	so take the whole binary value, complement it, and add 1.
01  Configuration Register (Read/Write) 16 bits, MSB first
10  TLOW Limit Register (Read/Write)
11  THIGH Limit Register (Read/Write)
*/

#define LCM300_PAGE_CMD 			0x00  // 8-bit read-only ???

// on/off and low/high margin voltage select
// default = 0x80
#define LCM300_OPERATION_CMD 		0x01  // 8-bit read/write

// WRITE PROTECT register 0x10 has four possible mutually-exclusive values
// 00h – Enable writing to all writeable commends
// 20h – Disables write except 10h, 01h, 00h, 02h and 21h commands
// 40h – Disables write except 10h, 01h, and 00h commends
// 80h – Disable write except 0x10h
// Note: Write access to the write protect register is always enabled!
#define LCM300_WRITE_PROTECT_CMD 	0x10  // 8-bit write control, read/write

// values for the WRITE PROTECT register
#define LCM300_WP_DISABLE_ALL		0x80  // disable write to all (except WP 0x10)
#define LCM300_WP_ENABLE_OPER_PAGE	0x40  // enable write to PAGE and OPERATION
#define LCM300_WP_ENABLE_OPER_PAGE_ONOFF_VOUT	0x20  // enable write to PAGE, OPERATION, ON_OFF_CONFIG, VOUT_COMMAND
#define LCM300_WP_ENABLE_ALL		0x00  // enable write to all writeable commands

// Set output voltage.
// LCM300 default 0x2FE6 = 24V
// LCM300U default 0x3F9E = 36V
// LCM300W default 0x2FEB = 48V
#define LCM300_VOUT_COMMAND_CMD 	0x21  // 16-bit output voltage set, read/write

// Read the max output voltage. 
// LCM300 0x3999 = 28.9V
// LCM300U 0x5666 = 43.2V
// LCM300W 0x3C0 = 60V
#define LCM300_VOUT_MAX_CMD 		0x24  // 16-bit max output voltage, read-only




class Systronix_LCM300
{
	protected:
		uint8_t		_base;								// base address for this instance; four possible values
		uint8_t		_pointer_reg;						// copy of the pointer register value so we know where it's pointing
		void		tally_errors (uint8_t);				// maintains the i2c_t3 error counters

	public:
		// Instance-specific properties
		/** Data for one instance of a LCM300 temp sensor.
		Extended 13-bit mode is assumed (12-bit mode is only there for compatibility with older parts)
		Error counters could be larger but then they waste more data in the typical case where they are zero.
		Errors peg at max value for the data type: they don't roll over.
		
		Maybe different structs for data values and part control
		**/
		struct
			{
			uint16_t	raw_temp;						// most recent
			uint16_t	t_high = 0xE480;				// preset to minimum temperature value (-55C in raw13 format)
			uint16_t	t_low = 0x4B00;  				// preset to max temperature value (150C in raw13 format)
			float		deg_c;        
			float		deg_f;
			bool		fresh;							// data is good and fresh TODO: how does one know that the data are not 'fresh'?
			} data;

		struct
			{
			uint8_t		ret_val;						// i2c_t3 library return value from most recent transaction
			uint32_t	incomplete_write_count;			// Wire.write failed to write all of the data to tx_buffer
			uint32_t	data_len_error_count;			// data too long
			uint32_t	rcv_addr_nack_count;			// slave did not ack address
			uint32_t	rcv_data_nack_count;			// slave did not ack data
			uint32_t	other_error_count;				// arbitration lost or timeout
			boolean		exists;							// set false after an unsuccessful i2c transaction
			} control;

		uint8_t BaseAddr;
														// i2c_t3 error counters

		void		setup (uint8_t base);				// constructor
		void		begin (void);
		uint8_t		init (uint16_t);					// device present and communicating detector

		float		raw13ToC (uint16_t raw13);			// temperature conversion functions
		float		raw13_to_F (uint16_t raw13);
		uint8_t		get_temperature_data (void);

		uint8_t		writePointer (uint8_t pointer);		// i2c bus dependent functions
		uint8_t		writeRegister (uint8_t pointer, uint16_t data);
		uint8_t		readRegister (uint16_t *data);

		uint8_t		readTempDegC (float *tempC);		// these functions not yet implemented; all return FAIL
		uint8_t		degCToRaw13 (uint16_t *raw13, float *tempC);
		uint8_t		getOneShotDegC (float *tempC);
		uint8_t		setModeOneShot (boolean mode);
		uint8_t		setModeContinuous (int8_t rate);

	private:

};

extern Systronix_LCM300 tmp102;

#endif /* SYSTRONIX_LCM300_h */
