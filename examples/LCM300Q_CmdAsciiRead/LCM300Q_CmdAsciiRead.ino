
/** ---------- LCM300Q Library Test Code Examples -------------------------------------------------

ASCII CMD READ

Read and display data from the ASCII commands of the LCM300Q
Mainly to test the library function commandAsciiRead

Controller is Teensy3

Copyright 2016 Systronix Inc www.systronix.com

------------------------------------------------------------------------------------------------**/
 
/** ---------- REVISIONS ----------

2016 Dec 21 bboyes  start

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

uint16_t good=0;
uint16_t bad=0;
uint16_t raw16;

char ascii[16];

uint8_t num_read;

uint8_t result;   // SUCCESS, FAIL, or ABSENT defined in library header

size_t read_cnt;

/* ========== SETUP ========== */
void setup(void) 
{

//  int8_t stat = -1;

  
  Serial.begin(115200);     // use max baud rate
  // Teensy3 doesn't reset with Serial Monitor as do Teensy2/++2, or wait for Serial Monitor window
  // Wait here for 10 seconds to see if we will use Serial Monitor, so output is not lost
  while((!Serial) && (millis()<10000));    // wait until serial monitor is open or timeout, which seems to fall through

  // start LCM300 library
  lcm300_58.setup(LCM300_BASE_MIN);

  Serial.print("LCM300Q Library Test Code at 0x");
  Serial.println(lcm300_58.BaseAddr, HEX);

  lcm300_58.begin();

  Serial.printf("Build %s - %s\r\n%s\r\n", __DATE__, __TIME__, __FILE__);

#if defined(__MKL26Z64__)
  Serial.println( "CPU is T_LC");
#elif defined(__MK20DX256__)
  Serial.println( "CPU is T_3.1/3.2");
#elif defined(__MK20DX128__)
  Serial.println( "CPU is T_3.0");
#elif defined(__MK64FX512__)
  Serial.println( "CPU is T_3.5");
#elif defined(__MK66FX1M0__)
  Serial.println( "CPU is T_3.6");
#endif
  Serial.print( "F_CPU =");   Serial.println( F_CPU );
  
#if defined I2C_T3_H 
  Serial.printf("Using i2c_t3 I2C library for Teensy\r\n");
#endif


   
//  int8_t flag = -1;  // I2C returns 0 if no error
  
  dtime = 2000;      // msec between samples, 1000 = 1 sec, 60,000 = 1 minute

  Serial.print(" Interval is ");
  Serial.print(dtime/1000);
  Serial.print(" sec, ");
 
  Serial.println("Setup Complete!");
  Serial.println(" "); 
  
  Serial.printf("sizeof ascii array is %i\r\n", sizeof(ascii));


}



/* ========== LOOP ========== */
void loop(void) 
{
//  int16_t temp0;
//  int8_t stat=-1;  // status flag
//  float temp;
  

  Serial.printf("@%u\r\n", millis()/1000);
  


  result = lcm300_58.commandAsciiRead(MFR_ID_CMD, 7, ascii);
  Serial.printf("mfr ID: %s\r\n\n", ascii);


  read_cnt = 16;
  result = lcm300_58.commandAsciiRead (MFR_MODEL_CMD, read_cnt, ascii);
  Serial.printf("model: %s\r\n\n", ascii);

  read_cnt = 16;
  result = lcm300_58.commandAsciiRead (MFR_REVISION_CMD, read_cnt, ascii);
  Serial.printf("revision: %s\r\n\n", ascii);
 
  read_cnt = 16;
  result = lcm300_58.commandAsciiRead (MFR_LOCATION_CMD, read_cnt, ascii);
  Serial.printf("location: %s\r\n\n", ascii); 

  read_cnt = 8;
  result = lcm300_58.commandAsciiRead (MFR_SERIAL_CMD, read_cnt, ascii);
  Serial.printf("Mfg Date: %s\r\n\n", ascii); 


  Serial.println();
  
  
  delay(dtime);
}











