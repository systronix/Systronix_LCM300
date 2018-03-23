
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

2016 Dec 01 bboyes  start

--------------------------------**/

#include <Arduino.h>
#include <Systronix_LCM300.h>	// best version of I2C library is #included by the library. Don't include it here!



/**
* debug level
* 0 = quiet, suppress everything
* 3 = max, even more or less trivial message are emitted
* 4 = emit debug info which checks very basic data conversion, etc
*/
byte DEBUG = 1;

uint16_t dtime;  // delay in loop


/**
Data for one instance of a LCM300 supply

**/
//struct data {
//  uint8_t address;    // I2C address, only the low 7 bits matter

//  uint16_t i2c_err_nak;  // total since startup
//  uint16_t i2c_err_rd;   // total read fails - data not there when expected
//};

//static data tmp48;      // LCM300Q at base address 0x48


Systronix_LCM300 lcm300_58;    // supply at default address

/* ========== SETUP ========== */
void setup(void) 
{

//  int8_t stat = -1;

  
  Serial.begin(115200);     // use max baud rate
  // Teensy3 doesn't reset with Serial Monitor as do Teensy2/++2, or wait for Serial Monitor window
  // Wait here for 10 seconds to see if we will use Serial Monitor, so output is not lost
  while((!Serial) && (millis()<10000));    // wait until serial monitor is open or timeout, which seems to fall through

  // start LCM300 library
  lcm300_58.setup (LCM300_BASE_MIN, Wire1, (char*)"Wire1");

	Serial.printf ("LCM300Q Library Test Code at 0x%.2X", lcm300_58.base_get());
//  Serial.println(lcm300_58.BaseAddr, HEX);

  lcm300_58.begin(I2C_PINS_29_30);
  lcm300_58.init();
   
//  int8_t flag = -1;  // I2C returns 0 if no error
  
  dtime = 5000;      // msec between samples, 1000 = 1 sec, 60,000 = 1 minute
  Serial.print(" Interval is ");
  Serial.print(dtime/1000);
  Serial.print(" sec, ");
 
  Serial.println("Setup Complete!");
  Serial.println(" "); 


  

}


uint16_t good=0;
uint16_t bad=0;
uint16_t raw16;

char ascii[32];

uint8_t num_read;

uint8_t result;   // SUCCESS, FAIL, or ABSENT defined in library header

size_t read_cnt;

/* ========== LOOP ========== */
void loop(void) 
{
//  int16_t temp0;
//  int8_t stat=-1;  // status flag
//  float temp;
  

  Serial.printf("@%u\r\n", millis()/1000);

  read_cnt = 16;
  result = lcm300_58.command_raw_read (MFR_ID_CMD, read_cnt, ascii);
  Serial.printf("mfr ID: %s\r\n\n", ascii);  
  
  read_cnt = 4;
  result = lcm300_58.command_raw_read (MFR_VOUT_MAX_CMD, read_cnt, ascii);
  Serial.printf("Mfr Vout Max: %s\r\n\n", ascii);  

  read_cnt = 2;
  result = lcm300_58.command_raw_read (VOUT_MODE_CMD, read_cnt, ascii);
  Serial.printf("Vout Mode: %s\r\n\n", ascii);  
  delay(50);
  
  read_cnt = 4;
  result = lcm300_58.command_raw_read (VOUT_COMMAND_CMD, read_cnt, ascii);
  Serial.printf("Vout Set Command: %s\r\n\n", ascii);  
  delay(50);
  
  read_cnt = 4;
  result = lcm300_58.command_raw_read (READ_VOUT_CMD, read_cnt, ascii);
  Serial.printf("Read Vout: %s\r\n\n", ascii);  
  delay(50);

  read_cnt = 4;
  result = lcm300_58.command_raw_read (READ_IOUT_CMD, read_cnt, ascii);
  Serial.printf("Iout: %s\r\n\n", ascii);  

  read_cnt = 4;
  result = lcm300_58.command_raw_read (MFR_IOUT_MAX_CMD, read_cnt, ascii);
  Serial.printf("%u Iout Max: %s\r\n\n", ascii);   

  read_cnt = 4;
  result = lcm300_58.command_raw_read (READ_TEMPERATURE_2_CMD, read_cnt, ascii);
  Serial.printf("Temperature 2: %s\r\n\n", ascii);  
  delay(50);

  read_cnt = 4;
  result = lcm300_58.command_raw_read (READ_FAN_SPEED_CMD, read_cnt, ascii);
  Serial.printf("Fan speed: %s\r\n\n", ascii); 

  Serial.println();
  
  
  delay(dtime);
}











