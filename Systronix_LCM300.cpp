/******************************************************************************/
/*!
	@file		Systronix_LCM300Q.cpp
	
	@author		B Boyes (Systronix Inc)
    @license	TBD (see license.txt)	
    @section	HISTORY


    v0.1	2016Dec01 bboyes Start based on LCM300Q library

*/
/******************************************************************************/
#include <Systronix_LCM300Q.h>	

//---------------------------< S E T U P >--------------------------------------------------------------------
/*!
	@brief  Instantiates a new LCM300Q class to use the given base address
	@todo	Test base address for legal range 0x48-0x4B
			Add constructor(void) for default address of 0x48
*/

void Systronix_LCM300Q::setup(uint8_t base)
	{
	_base = base;
	BaseAddr = base;

//	struct data _data;			// instance of the data struct - not used
//	_data.address = _base;		// struct - not used
	}


//---------------------------< B E G I N >--------------------------------------------------------------------
/*!
	@brief  Join the I2C bus as a master
*/

void Systronix_LCM300Q::begin(void)
	{
	Wire.begin();	// join I2C as master
	}


//---------------------------< I N I T >----------------------------------------------------------------------
//
// Attempts to write the pointer register.  If successful, sets control.exists true, else false.
//

uint8_t Systronix_LCM300Q::init (uint16_t config)
	{
	uint8_t written;
	
	Wire.beginTransmission (_base);						// base address
	written = Wire.write (LCM300Q_CONF_REG_PTR);			// pointer in 2 lsb
	written += Wire.write ((uint8_t)(config >> 8));		// write MSB of configuration
	written += Wire.write ((uint8_t)(config & 0x00FF));	// write LSB of configuration

	if (3 != written)
		{
		control.exists = false;							// unsuccessful i2c_t3 library call
		return FAIL;
		}
	
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

void Systronix_LCM300Q::tally_errors (uint8_t error)
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


//---------------------------< R A W 1 3 T O C >--------------------------------------------------------------
/*!
	@brief  Convert raw 13-bit temperature to float deg C
			handles neg and positive values specific to LCM300Q extended mode 
			13-bit temperature data

	@TODO instead pass a pointer to the float variable? and return error if value out of bounds
*/
//
// receives uint16_t raw13 argument that should be an int16_t.  But, because readRegister() is a general
// purpose function for reading all of the 16 bite registers we get around that by casting the uint16_t to
// int16_t before we use the value.
//
// TODO: take some time to consider if it is worthwhile to have a separate function that just fetches the
// temperature register.  Obvious downside (if it is a downside) is that such a function would necessarily need
// to set the pointer register every time the temperature is read.
//

float Systronix_LCM300Q::raw13ToC (uint16_t raw13)
	{
	uint8_t		shift = (raw13 & 1) ? 3 : 4;		// if extended mode shift 3, else shift 4
	return 0.0625 * ((int16_t)raw13 >> shift);
	}


//---------------------------< R A W 1 3 _ T O _ F >----------------------------------------------------------
//
// Convert raw 13-bit LCM300Q temperature to degrees Fahrenheit.
//

float Systronix_LCM300Q::raw13_to_F (uint16_t raw13)
	{
	return (raw13ToC (raw13) * 1.8) + 32.0;
	}


//---------------------------< G E T _ T E M P E R A T U R E _ D A T A >--------------------------------------
//
// Gets current temperature and fills the data struct with the various temperature info
//

uint8_t Systronix_LCM300Q::get_temperature_data (void)
	{
	
	if (_pointer_reg)									// if not pointed at temperature register
		if (writePointer (LCM300Q_TEMP_REG_PTR))			// attempt to point it
			return FAIL;								// attempt failed; quit
	
	if (readRegister (&data.raw_temp))					// attempt to read the temperature
		return FAIL;									// attempt failed; quit
	
	data.t_high = max((int16_t)data.raw_temp, (int16_t)data.t_high);	// keep track of min/max temperatures
	data.t_low = min((int16_t)data.t_low, (int16_t)data.raw_temp);

	data.deg_c = raw13ToC (data.raw_temp);					// convert to human-readable forms
	data.deg_f = raw13_to_F (data.raw_temp);
	
	return SUCCESS;
	}


//---------------------------< W R I T E P O I N T E R >------------------------------------------------------
/**
Write to a LCM300Q register
Start with slave address, as in any I2C transaction.
Next byte must be the Pointer Register value, in 2 lsbs
If all you want to do is set the Pointer Register for subsequent read(s)
then this 2-byte write cycle is complete.

Data bytes in the write are optional but must always follow the Pointer Register write byte.
The last value written to the Pointer Register persists until changed.

**/

uint8_t Systronix_LCM300Q::writePointer (uint8_t pointer)
	{
	if (!control.exists)								// exit immediately if device does not exist
		return ABSENT;

	_pointer_reg = pointer;								// keep a copy for use by other functions
	Wire.beginTransmission (_base);						// base address
	control.ret_val = Wire.write (_pointer_reg);		// pointer in 2 lsb
	if (1 != control.ret_val)
		{
		control.ret_val = 0;
		tally_errors (control.ret_val);					// increment the appropriate counter
		return FAIL;
		}

	control.ret_val = Wire.endTransmission();
  	if (SUCCESS == control.ret_val)
		return SUCCESS;
	tally_errors (control.ret_val);						// increment the appropriate counter
	return FAIL;										// calling function decides what to do with the error
	}


//---------------------------< W R I T E R E G I S T E R >----------------------------------------------------
/**
Param pointer is the LCM300Q register into which to write the data
data is the 16 bits to write.
returns 0 if no error, positive values for NAK errors
**/

uint8_t Systronix_LCM300Q::writeRegister (uint8_t pointer, uint16_t data)
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


//---------------------------< R E A D R E G I S T E R >------------------------------------------------------
/**
  Read the 16-bit register addressed by the current pointer value, store the data at the location passed
  
  return 0 if no error, positive bytes read otherwise.
*/

uint8_t Systronix_LCM300Q::readRegister (uint16_t *data)
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


//---------------------------< R E A D T E M P D E G C >------------------------------------------------------
/**
Read the most current temperature already converted and present in the LCM300Q temperature registers

In continuous mode, this could be one sample interval old
In one shot mode this data is from the last-requested one shot conversion
**/
uint8_t Systronix_LCM300Q::readTempDegC (float *tempC) 
	{
	return FAIL;
	}



//---------------------------< D E G C T O R A W 1 3 >--------------------------------------------------------
/**
Convert deg C float to a raw 13-bit temp value in LCM300Q format.
This is needed for Th and Tl registers as thermostat setpoint values

return 0 if OK, error codes if float is outside range of LCM300Q
**/

uint8_t Systronix_LCM300Q::degCToRaw13 (uint16_t *raw13, float *tempC)
	{
	return FAIL;
	}


//---------------------------< G E T O N E S H O T D E G C>---------------------------------------------------
/**
Trigger a one-shot temperature conversion, wait for the new value, about 26 msec, and update 
the variable passed.

If the LCM300Q is in continuous conversion mode, this places the part in One Shot mode, 
triggers the conversion, waits for the result, updates the variable, and leaves the LCM300Q in one shot mode.

returns 0 if no error
**/

uint8_t Systronix_LCM300Q::getOneShotDegC (float *tempC)
	{
	return FAIL;
	}


//---------------------------< S E T M O D E O N E S H O T >--------------------------------------------------
/**
Set the LCM300Q mode to one-shot, with low power sleep in between

mode: set to One Shot if true. 
If false, sets to continuous sampling mode at whatever sample rate was last set.

returns: 0 if successful
**/

uint8_t Systronix_LCM300Q::setModeOneShot (boolean mode)
	{
	return FAIL;
	}


//---------------------------< S E T M O D E C O N T I N U O U S >--------------------------------------------
/**
Set LCM300Q mode to continuous sampling at the rate given.

rate: must be one of the manifest constants such as LCM300Q_CFG_RATE_1HZ
if rate is not one of the four supported, it is set to the default 4 Hz

returns: 0 if successful
**/

uint8_t Systronix_LCM300Q::setModeContinuous (int8_t rate)
	{
	return FAIL;
	}
