#ifndef SYSTRONIX_LCM300_h
#define SYSTRONIX_LCM300_h


/**************************************************************************************************/
/*!
	@file		Systronix_LCM300.h
	
	@author		B Boyes (Systronix Inc)
    @license	TBD (see license.txt)	
    @section	HISTORY


	v0.2	2018Mar22 bboyes finishing up thanks to some tech support from Artesyn
    v0.1	2016Dec01 bboyes Start based on LCM300 library

*/
/**************************************************************************************************/

/** TODO LIST

Read linear data such as voltage, with exponent
Read ascii data, based on first byte, null-terminate returned string

Add UI screens to report:

Power model and revision, e.g. "LCM300Q-T AK"  // 0x9A and 0x9B

**/

/***************************************************************************************************
	Some Explanatory and hopefully helful comments

LCM300 Data is here, including link to current Technical Reference Note: 
https://www.artesyn.com/power/power-supplies/websheet/491/lcm300-series 

The LCM300 Technical Reference Note assumes familiarity with, and is largely based on, the PMBus 
specification. See PMBus.org. It is in multiple sections. Important details such as the VOUT_MODE
and "linear" data format are not in the LCM300 TRN but in the PMBus documents. Fair enough. But
referring to the appropriate PMBus document at appropriate spots in LCM300 TRN would have been helpful.

There are numerous errors in the LCM300 TRN. We have reported many of these back in 2016 but nothing
appears to have been done about them.

In addition the actual implementation of the LCM300 PMBus interface is not consistent with the TRN.

	Watch out for these "gotchas"

- ASCII format data, details not documented in LCM300 TRN, such as:
	The first byte of the value is the length of the string.
	Strings are not null-terminated.

***************************************************************************************************/



#include <Arduino.h>

#if defined (__MK20DX256__) || defined (__MK20DX128__) 	// Teensy 3.1 or 3.2 || Teensy 3.0
#include <i2c_t3.h>	// for Teensy 3 family use optimized and more robust library
#else
#include <Wire.h>	// for AVR I2C library
#endif

#define		SUCCESS	0
#define		FAIL	(~SUCCESS)
#define		ABSENT	0xFD

#define		WR_INCOMPLETE		11
#define		SILLY_PROGRAMMER	12


/** --------  Register Addresses --------

*/

#define LCM300_BASE_MIN 			0x58  // 7-bit address not including R/W bit
#define LCM300_BASE_MAX 			0x5F  // 7-bit address not including R/W bit

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

// VOUT_MODE, per PMBus specs
// 3 msb are the mode. 
// 000 = linear mode, 
// 5 lsb are a signed 2's complement exponent
#define VOUT_MODE_CMD		0x20	// read read-only, 0x17 for our supply

// Set output voltage.
// LCM300 default 0x2FE6 = 24V in Tech Note but these may all be wrong!
// LCM300U default 0x3F9E = 36V
// LCM300W default 0x2FEB = 48V
#define VOUT_COMMAND_CMD 	0x21  // 16-bit output voltage set, read/write, returns 0x3033 and 0x3038

// Read the max output voltage. 
// LCM300 0x3999 = 28.9V
// LCM300U 0x5666 = 43.2V
// LCM300W 0x3C0 = 60V
#define VOUT_MAX_CMD 		0x24	// 16-bit max output voltage, read-only, returns 0x3999, same as 0xA5

#define FAN_COMMAND_1		0x3B	// 2 byte linear, but only ever returns 0

#define READ_VOUT_CMD		0x8B	// 2-byte linear format, 0x3014 = 24.04, 0x3019 = 24.05, 0x301E = 24.06
#define READ_IOUT_CMD		0x8C	// 16-bit measured output current, 5% accurate at load of 40% and greater
#define READ_TEMPERATURE_2_CMD	0x8D	// 2 bytes, Linear-11 format

#define READ_FAN_SPEED_CMD	0x90 	// reads 2595 raw 0x0A23 when stalled, 6873 running, not divided by exponent

#define READ_POUT_CMD		0x96	// 2 byte linear, output power in watts

// ASCII data
// MFR ID and product data area, 0x99-0x9F ASCII data 
#define MFR_ID_CMD			0x99	// ASCII such "Emerson" or maybe someday "Artesyn"
#define MFR_MODEL_CMD		0x9A	// ASCII such as "LCM300Q-T"
#define MFR_REVISION_CMD	0x9B	// ASCII such as "0A"
#define MFR_LOCATION_CMD	0x9C	// ASCII such as "Philippines"

#define MFR_DATE_CMD		0x9D	// ASCII such as "YYMMDD" Confusion across different models: YYWWDD? YYWW? YYMMDD?

// MFR_SERIAL_CMD returns a length value of 0x0d, 13 bytes
#define MFR_SERIAL_CMD		0x9E	// ASCII such as "123456789ABCD". Length "varies" re: TRN 1.5 p63

#define PMBUS_REVISION_CMD	0x98	// binary, 1 byte unsigned 0x22 = PMBus revision 2.2

#define COEFFICIENTS_CMD	0x30 	// not implememented, returns 0xFF. For LCM300 m=1, b=0, R=0

// 0xA0-0xAB "linear" data format
#define MFR_VOUT_MIN_CMD	0xA4	// linear returns 0x2666, should be 19.2V
#define MFR_VOUT_MAX_CMD	0xA5	// linear returns 0x3999, should be 28.8V
#define MFR_IOUT_MAX_CMD	0xA6	// linear returns 0xD3A0 = 14.5 amps




class Systronix_LCM300
{
	protected:
		uint8_t		_base;								// base address for this instance; four possible values
		void		tally_transaction (uint8_t);		// maintains the i2c_t3 error counters

		char* 		_wire_name = (char*)"empty";
		i2c_t3		_wire = Wire;						// why is this assigned value = Wire? [bab]

		uint8_t 	_vout_mode;							// the 3 msb of VOUT_MODE, shifted to 3 lsb of this value
		uint8_t		_linear_exponent;					// the 5 lsb of VOUT_MODE in signed 2's complement

	public:
		struct
			{
			bool		exists;							// set false after an unsuccessful i2c transaction
			uint8_t		error_val;						// the most recent error value, not just SUCCESS or FAIL
			uint32_t	incomplete_write_count;			// Wire.write failed to write all of the data to tx_buffer
			uint32_t	data_len_error_count;			// data too long
			uint32_t	timeout_count;					// slave response took too long
			uint32_t	rcv_addr_nack_count;			// slave did not ack address
			uint32_t	rcv_data_nack_count;			// slave did not ack data
			uint32_t	arbitration_lost_count;
			uint32_t	buffer_overflow_count;
			uint32_t	other_error_count;				// from endTransmission there is "other" error
			uint32_t	unknown_error_count;
			uint32_t	data_value_error_count;			// I2C message OK but value read was wrong; how can this be?
			uint32_t	silly_programmer_error;			// I2C address to big or something else that "should never happen"
			uint64_t	total_error_count;				// quick check to see if any have happened
			uint64_t	successful_count;				// successful access cycle
			} error;

		char*		wire_name;							// name of Wire, Wire1, etc in use

		uint8_t		setup (uint8_t base, i2c_t3 wire, char* name);				// constructor

		void		begin (i2c_pins pins);
		void		begin (void)						// default begin() (Wire0)
						{begin (I2C_PINS_18_19);}

		uint8_t		init (void);						// device present and communicating detector

		void		reset_bus (void);
		uint32_t	reset_bus_count_read (void);

		uint8_t		base_get (void);

		// This is the a currently-used function
		uint8_t 	command_raw_read (int cmd, size_t count, char *data);

		// Read a command register and convert the data using "linear" format
		uint8_t 	command_linear_read16 (int cmd, uint16_t data, bool debug=false);

		// Note that many LCM300 ASCII values do not match the LCM300 TRN 1.5 defaults
		// Also many aspects are not documented in the LCM300 TRN. This function works correctly.
		uint8_t 	command_ascii_read (int cmd, size_t length, char *data, bool debug=false);

		// These are not currently used and may have no purpose since you need to pass an address with every write or read
		uint8_t		register_write (uint8_t pointer, uint16_t data);	// write 16 bits of data
		uint8_t		register_read (uint16_t *data);						// read 16 bits of data

	private:

};

//extern Systronix_LCM300 lcm300;

#endif /* SYSTRONIX_LCM300_h */
