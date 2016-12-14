
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


Systronix_LCM300 lcm300_5F;    // supply at default address

/* ========== SETUP ========== */
void setup(void) 
{

  int8_t stat = -1;

  
  Serial.begin(115200);     // use max baud rate
  // Teensy3 doesn't reset with Serial Monitor as do Teensy2/++2, or wait for Serial Monitor window
  // Wait here for 10 seconds to see if we will use Serial Monitor, so output is not lost
  while((!Serial) && (millis()<10000));    // wait until serial monitor is open or timeout, which seems to fall through

  // start LCM300 library
  lcm300_5F.setup(LCM300_BASE_MAX);
  
  Serial.print("LCM300Q Library Test Code at 0x");
  Serial.println(lcm300_5F.BaseAddr, HEX);

   lcm300_5F.begin();
   
//  int8_t flag = -1;  // I2C returns 0 if no error
  
  dtime = 1000;      // msec between samples, 1000 = 1 sec, 60,000 = 1 minute
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

size_t read_cnt;

/* ========== LOOP ========== */
void loop(void) 
{
//  int16_t temp0;
  int8_t stat=-1;  // status flag
  float temp;
  

  Serial.printf("@%u\r\n", millis()/1000);
  
  

  read_cnt = 4;
  num_read = lcm300_5F.commandAsciiRead (PMBUS_REVISION_CMD, read_cnt, ascii);
  Serial.printf("%u PMBus rev bytes read: %s\r\n", num_read, ascii);

  read_cnt = 16;
  num_read = lcm300_5F.commandAsciiRead (MFR_ID_CMD, read_cnt, ascii);
  Serial.printf("%u mfr ID bytes read: %s\r\n", num_read, ascii);

  read_cnt = 16;
  num_read = lcm300_5F.commandAsciiRead (MFR_MODEL_CMD, read_cnt, ascii);
  Serial.printf("%u model bytes read: %s\r\n", num_read, ascii);

  read_cnt = 16;
  num_read = lcm300_5F.commandAsciiRead (MFR_REVISION_CMD, read_cnt, ascii);
  Serial.printf("%u revision bytes read: %s\r\n", num_read, ascii);
 
  read_cnt = 16;
  num_read = lcm300_5F.commandAsciiRead (MFR_LOCATION_CMD, read_cnt, ascii);
  Serial.printf("%u location bytes read: %s\r\n", num_read, ascii); 



  Serial.println();
  
  
  delay(dtime);
}











