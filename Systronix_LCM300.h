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



#include <Arduino.h>

#if defined (__MK20DX256__) || defined (__MK20DX128__) 	// Teensy 3.1 or 3.2 || Teensy 3.0
#include <i2c_t3.h>	// for Teensy 3 family use optimized and more robust library
#else
#include <Wire.h>	// for AVR I2C library
#endif

#define		SUCCESS	0
#define		FAIL	0xFF
#define		ABSENT	0xFD

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

// Set output voltage.
// LCM300 default 0x2FE6 = 24V
// LCM300U default 0x3F9E = 36V
// LCM300W default 0x2FEB = 48V
#define LCM300_VOUT_COMMAND_CMD 	0x21  // 16-bit output voltage set, read/write

// Read the max output voltage. 
// LCM300 0x3999 = 28.9V
// LCM300U 0x5666 = 43.2V
// LCM300W 0x3C0 = 60V
#define LCM300_VOUT_MAX_CMD 		0x24	// 16-bit max output voltage, read-only

#define LCM300_READ_VOUT_CMD		0x8B	// 16-bit measured output voltage
#define LCM300_READ_IOUT_CMD		0x8C	// 16-bit measured output current

// MFR ID and product data area, 0x99-0x9F ASCII data, 0xA0-0xAB "linear" data format 
// 0x99-0x
#define MFR_ID_CMD					0x099	// 7 bytes should be ASCII "Artesyn"
#define MFR_MODEL_CMD				0x09A	// 7 bytes such as "LCM300Q"
#define MFR_REVISION_CMD			0x09B	// 2 bytes such as "0A"
#define MFR_LOCATION_CMD			0x09C	// 6 bytes

class Systronix_LCM300
{
	protected:
		uint8_t		_base;								// base address for this instance; four possible values
		uint8_t		_pointer_reg;						// copy of the pointer register value so we know where it's pointing
		void		tally_errors (uint8_t);				// maintains the i2c_t3 error counters

	public:
		// Instance-specific properties
		/** Data for one instance of a LCM300 supply; up to 8 could be on the same network
		Maybe different structs for data values and control?
		**/
		struct
			{

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

		uint8_t 	commandAsciiRead (int cmd, size_t count, char *data);

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

//extern Systronix_LCM300 lcm300;

#endif /* SYSTRONIX_LCM300_h */
