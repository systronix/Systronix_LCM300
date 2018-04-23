
/** ---------- LCM300Q Library Test Code ------------------------

Controller is Teensy3

Copyright 2016 Systronix Inc www.systronix.com

NOTES ABOUT I2C library

Wire.endTransmission() seems to be only intended for use with a master write.
Wire.requestFrom() is used to get bytes from a slave, with read().
beginTransmission() followed by read() does not work. Slave address gets sent, then nothing else. As if the read commands get ignored.
I guess reads are not a "Transmission".

So to read three bytes, do a requestFrom(address, 2, true). << insists the first param is int.
Compiler has major whines if called as shown in the online Wire reference.

**/
 
/** ---------- REVISIONS ----------

2016 Dec 01 bboyes	start

--------------------------------**/

#include <Arduino.h>
#include <Systronix_LCM300.h>	// best version of I2C library is #included by the library. Don't include it here!

#define		PIE_ONLY



uint16_t dtime;  // delay in loop
uint8_t result;   // SUCCESS, FAIL, or ABSENT defined in library header
bool verbose = false; // don't print out detailed info about the PMBus command transaction

Systronix_LCM300 lcm300_58;    // supply at default address

//---------------------------< S E T U P >--------------------------------------------------------------------
//
//
//

void setup(void) 
	{
	Serial.begin(115200);     // use max baud rate
	// Teensy3 doesn't reset with Serial Monitor as do Teensy2/++2, or wait for Serial Monitor window
	// Wait here for 10 seconds to see if we will use Serial Monitor, so output is not lost
	while((!Serial) && (millis()<10000));    // wait until serial monitor is open or timeout, which seems to fall through

	// start LCM300 library
	lcm300_58.setup (LCM300_BASE_MIN, Wire1, (char*)"Wire1");

	Serial.printf ("LCM300Q Library Test Code at 0x%.2X\n", lcm300_58.base_get());

	lcm300_58.begin(I2C_PINS_29_30);
	if (SUCCESS != lcm300_58.init())
		{
		Serial.printf ("lcm300:init() fail; halted");
		while(1);
		}
  
	dtime = 5000;      // msec between samples, 1000 = 1 sec, 60,000 = 1 minute
	Serial.printf (" Interval is %d sec, setup complete\n", dtime/1000);
//	Serial.print(dtime/1000);
//	Serial.print(" sec, ");

//	Serial.println("Setup Complete!");
//	Serial.println(" "); 
	}


/* ========== LOOP ========== */

time_t		loop_time;
time_t		last_loop_time;
time_t		interval;
uint32_t	number_of_samples;

void loop(void) 
	{
	last_loop_time = loop_time;
	loop_time = millis();
	interval = loop_time - last_loop_time;
	Serial.printf("@%u\r\n", loop_time/1000);

#ifndef PIE_ONLY
	Serial.printf("\nMfr ID:\n");
	result = lcm300_58.command_raw_read (MFR_ID_CMD, verbose);
	Serial.printf ("\tMfr ID: %s\n", &lcm300_58.cmd_response.as_array[1]);

	Serial.printf("\nMfr model:\n");
	result = lcm300_58.command_raw_read (MFR_MODEL_CMD, verbose);
	Serial.printf ("\tMfr model: %s\n", &lcm300_58.cmd_response.as_array[1]);

	Serial.printf("\nMfr revision:\n");
	result = lcm300_58.command_raw_read (MFR_REVISION_CMD, verbose);
	Serial.printf ("\tMfr revision: %s\n", &lcm300_58.cmd_response.as_array[1]);

	Serial.printf("\nMfr location:\n");
	result = lcm300_58.command_raw_read (MFR_LOCATION_CMD, verbose);
	Serial.printf ("\tMfr location: %s\n", &lcm300_58.cmd_response.as_array[1]);

	Serial.printf("\nMfr date:\n");	// pointless; returns string 'YYMMDD'
	result = lcm300_58.command_raw_read (MFR_DATE_CMD, verbose);
	Serial.printf ("\tMfr date: %s\n", &lcm300_58.cmd_response.as_array[1]);

	Serial.printf("\nMfr serial:\n");	// pointless; returns string '123456789ABCD'
	result = lcm300_58.command_raw_read (MFR_SERIAL_CMD, verbose);
	Serial.printf ("\tMfr serial: %s\n", &lcm300_58.cmd_response.as_array[1]);

	Serial.printf("\nMfr Vout min command:\n");
	result = lcm300_58.command_raw_read (MFR_VOUT_MIN_CMD, verbose);
	Serial.printf ("\tMfr Vout min command: %.2fV\n", lcm300_58.raw_voltage_to_float (lcm300_58.cmd_response.as_word));

	Serial.printf("\nMfr Vout max command:\n");
	result = lcm300_58.command_raw_read (MFR_VOUT_MAX_CMD, verbose);
	Serial.printf ("\tMfr Vout max command: %.2fV\n", lcm300_58.raw_voltage_to_float (lcm300_58.cmd_response.as_word));

	Serial.printf("\nMfr Iout max command:\n");
	result = lcm300_58.command_raw_read (MFR_IOUT_MAX_CMD, verbose);
	Serial.printf ("\tMfr Iout max command: %.2fA\n", lcm300_58.pmbus_literal_to_float (lcm300_58.cmd_response.as_word));



	Serial.printf("\nVout Mode:\n");
	result = lcm300_58.command_raw_read (VOUT_MODE_CMD, verbose);
	Serial.printf ("\tvout mode: 0x%.2X\n", lcm300_58.cmd_response.as_byte);

	Serial.printf("\nPMBus revision:\n");
	result = lcm300_58.command_raw_read (PMBUS_REVISION_CMD, verbose);
	Serial.printf ("\tPMBus revision: 0x%.2X\n", lcm300_58.cmd_response.as_byte);


	
	Serial.printf("\nVout Command:\n");
	result = lcm300_58.command_raw_read (VOUT_COMMAND_CMD, verbose);
	Serial.printf ("\tVout command: %.2fV\n", lcm300_58.raw_voltage_to_float (lcm300_58.cmd_response.as_word));

	Serial.printf("\nVout max command:\n");
	result = lcm300_58.command_raw_read (VOUT_MAX_CMD, verbose);
	Serial.printf ("\tVout max command: %.2fV\n", lcm300_58.raw_voltage_to_float (lcm300_58.cmd_response.as_word));
#endif

	Serial.printf("\nRead Vout Command:\n");
	result = lcm300_58.command_raw_read (READ_VOUT_CMD, verbose);
	Serial.printf ("\tRead Vout: %.2fV\n", lcm300_58.raw_voltage_to_float (lcm300_58.cmd_response.as_word));

	Serial.printf("\nRead Iout Command:\n");
	result = lcm300_58.command_raw_read (READ_IOUT_CMD, verbose);
	Serial.printf ("\tRead Iout: %.2fA\n", lcm300_58.pmbus_literal_to_float (lcm300_58.cmd_response.as_word));

	Serial.printf("\nRead Pout Command:\n");
	result = lcm300_58.command_raw_read (READ_POUT_CMD, verbose);
	Serial.printf ("\tRead Pout: %.2fW\n", lcm300_58.pmbus_literal_to_float (lcm300_58.cmd_response.as_word));

	Serial.printf("\nRead Eout Command:\n");
	result = lcm300_58.command_raw_read (READ_EOUT_CMD, true);

	number_of_samples = (uint32_t)(*(uint32_t*)&lcm300_58.cmd_response.as_array[4] & 0x00FFFFFF) - lcm300_58.eout_data.last_sample_count;
	Serial.printf ("\t\tsamples per %dmS: %d (%.4f/sec)\n", interval, number_of_samples, (float)(number_of_samples/(interval/1000.0)));

	lcm300_58.pmbus_average_power ();

	Serial.printf ("\t\taccumulator: %d\n", lcm300_58.eout_data.accumulator);
	Serial.printf ("\t\trollover count: %d\n", lcm300_58.eout_data.rollover_count);
	Serial.printf ("\t\tsample count: %d\n", lcm300_58.eout_data.sample_count);
	Serial.printf ("\t\tenergy count: %u\n", lcm300_58.eout_data.energy_count);
	if (500 < lcm300_58.eout_data.average_power)
		Serial.printf ("\t\taverage power: rollover detected\n");
	else
		Serial.printf ("\t\taverage power: %u\n", lcm300_58.eout_data.average_power);

#ifndef PIE_ONLY
	Serial.printf("\nTemperature 2:\n");
	result = lcm300_58.command_raw_read (READ_TEMPERATURE_2_CMD, verbose);
	Serial.printf ("\tTemperature 2: %.2fC\n", lcm300_58.pmbus_literal_to_float (lcm300_58.cmd_response.as_word));
#endif

	Serial.printf("\nFan speed:\n"); 
	result = lcm300_58.command_raw_read (READ_FAN_SPEED_CMD, verbose);
	Serial.printf ("\tFan speed: %.0frpm (linear)\n", lcm300_58.pmbus_literal_to_float (lcm300_58.cmd_response.as_word));	// data sheet specifies linear
	Serial.printf ("\tFan speed: %drpm (direct)\n", (int16_t)lcm300_58.cmd_response.as_word);										// this is 
	
#ifndef PIE_ONLY
	Serial.printf("\nStatus:\n");
	result = lcm300_58.command_raw_read (STATUS_BYTE_CMD, verbose);
	Serial.printf ("\t0x78: 0x%.2X\n", lcm300_58.cmd_response.as_byte);

	result = lcm300_58.command_raw_read (STATUS_WORD_CMD, verbose);
	Serial.printf ("\t0x79: 0x%.4X\n", lcm300_58.cmd_response.as_word);

	result = lcm300_58.command_raw_read (STATUS_VOUT_CMD, verbose);
	Serial.printf ("\t0x7A: 0x%.2X\n", lcm300_58.cmd_response.as_byte);

	result = lcm300_58.command_raw_read (STATUS_IOUT_CMD, verbose);
	Serial.printf ("\t0x7B: 0x%.2X\n", lcm300_58.cmd_response.as_byte);

	result = lcm300_58.command_raw_read (STATUS_TEMP_CMD, verbose);
	Serial.printf ("\t0x7D: 0x%.2X\n", lcm300_58.cmd_response.as_byte);
#endif

	Serial.printf ("\n");


	delay(dtime);
	}











