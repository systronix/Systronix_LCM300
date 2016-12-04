
/** ---------- LCM300Q Library Test Code ------------------------

Controller is Teensy++ 2.0 but will also work with Arduino
We are running at 3.3V and therefore 8 MHz

Copyright 2013-2016 Systronix Inc www.systronix.com

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


//Systronix_LCM300 tmp102_48(0x48);    // We can pass constructor a value

/* ========== SETUP ========== */
void setup(void) 
{
  uint16_t raw16=0;  // place to put what we just read
//  uint16_t wrt16=0;  // temp write variable
  int8_t stat = -1;
//  int16_t temp_int16 = 0;
  
  Serial.begin(115200);     // use max baud rate
  // Teensy3 doesn't reset with Serial Monitor as do Teensy2/++2, or wait for Serial Monitor window
  // Wait here for 10 seconds to see if we will use Serial Monitor, so output is not lost
  while((!Serial) && (millis()<10000));    // wait until serial monitor is open or timeout, which seems to fall through
  
  Serial.print("LCM300Q Library Test Code at 0x");
  Serial.println(tmp102_48.BaseAddr, HEX);
   
//  int8_t flag = -1;  // I2C returns 0 if no error
  
 
  // start LCM300 library


  dtime = 1000;      // msec between samples, 1000 = 1 sec, 60,000 = 1 minute
  Serial.print(" Interval is ");
  Serial.print(dtime/1000);
  Serial.print(" sec, ");
 
  Serial.println("Setup Complete!");
  Serial.println(" "); 
  
  if (1 == DEBUG)
  {
    Serial.println("sec deg C");
  }
}


uint16_t good=0;
uint16_t bad=0;
uint16_t raw16;

/* ========== LOOP ========== */
void loop(void) 
{
//  int16_t temp0;
  int8_t stat=-1;  // status flag
  float temp;
  
  Serial.print("@");
  Serial.print(millis()/1000);
  Serial.print(" ");
  
  if (!fake)  // get real temperature data from sensor
  {
  //  Serial.print("good:");
  //  Serial.print(good);
  //  Serial.print(" ");
    if (bad > 0) 
    {
      Serial.print(" bad:");
      Serial.print(bad);
      Serial.print(" ");
    }
  
    if (DEBUG >=3)
    {
      Serial.print("ALpin:");
      Serial.print(digitalRead(19));
      Serial.print(" ");
    }
  
    stat = tmp102_48.writePointer(LCM300Q_CONF_REG_PTR);
    if (DEBUG >=3)
    {
      stat = tmp102_48.readRegister (&raw16);
      Serial.print("CFG:");
      Serial.print(raw16, HEX);
      Serial.print(" ");
    }
  
    configOptions |= LCM300Q_CFG_OS;        // start One Shot conversion
    stat = tmp102_48.writeRegister (LCM300Q_CONF_REG_PTR, configOptions);
  
    if (DEBUG >=2)
    {  
      stat = tmp102_48.readRegister (&raw16);
      Serial.print("CFG:");
      Serial.print(raw16, HEX);
      Serial.print(" ");
    }
    // pointer set to read temperature
    stat = tmp102_48.writePointer(LCM300Q_TEMP_REG_PTR); 
    // read two bytes of temperature
    stat = tmp102_48.readRegister (&rawtemp);
//    if (2==stat) good++;
    if (SUCCESS==stat) good++;
    else bad++;
  } // if !fake 
  else rawtemp = faketemp;    // fresh simulated value
  
  if (DEBUG >= 2)
  {
    Serial.print ("Raw16:0x");
    if (0==(rawtemp & 0xF000)) Serial.print("0");
    Serial.print(rawtemp, HEX);
    Serial.print (" ");
  }

  temp = tmp102_48.raw13ToC(rawtemp);
  
  temperature = temp;  // for Ethernet client
  
  if (DEBUG >= 2)
  {
    Serial.print("ms13:0x");
    Serial.print(rawtemp, HEX);
    Serial.print (" ");
  }
  
  if (fake) Serial.print (temp,4);  // no rounding of simulated data
  else Serial.print (temp,2);       // 2 dec pts good enough for real data 0.0625 deg C per count
  
  Serial.print (" C ");
  
  if (DEBUG >= 2)
  {
    Serial.print(rawtemp, DEC);
    Serial.print ("D ");
  }
  
  // test with all values of simulated rawtemp data
  if (fake) 
  {
    // faketemp raw change of 0x280 is 5 deg C
    faketemp -= 0x280;  
    // 0xE480 is min legal value
    if (temp <= -55) 
    {
      faketemp = 0x4B00;  // if min then reset to max    
      
      fake = false;      // switch to real data after full cycle of simulated
      dtime = 2000;
      Serial.println();
      Serial.print ("Changing to real data");
    }
  }
  
  Serial.println();
  
  
  delay(dtime);
}



/**
Read the most current temperature already converted and present in the LCM300Q temperature registers

In continuous mode, this could be one sample interval old
In one shot mode this data is from the last-requested one shot conversion
**/
uint8_t readTempDegC (float *tempC) 
{
    return 1;
}



/**
Convert deg C float to a raw 13-bit temp value in LCM300Q format.
This is needed for Th and Tl registers as thermostat setpoint values

return 0 if OK, error codes if float is outside range of LCM300Q
**/
int8_t degCToRaw13 (uint16_t *raw13, float *tempC)
{
    return 1;
}



/**
Trigger a one-shot temperature conversion, wait for the new value, about 26 msec, and update 
the variable passed.

If the LCM300Q is in continuous conversion mode, this places the part in One Shot mode, 
triggers the conversion, waits for the result, updates the variable, and leaves the LCM300Q in one shot mode.

returns 0 if no error
**/
uint8_t getOneShotDegC (float *tempC)
{
    return 1;
}


/**
Set the LCM300Q mode to one-shot, with low power sleep in between

mode: set to One Shot if true. 
If false, sets to continuous sampling mode at whatever sample rate was last set.

returns: 0 if successful
**/
int8_t setModeOneShot (boolean mode)
{
    return 1;
}

/**
Set LCM300Q mode to continuous sampling at the rate given.

rate: must be one of the manifest constants such as LCM300Q_CFG_RATE_1HZ
if rate is not one of the four supported, it is set to the default 4 Hz

returns: 0 if successful
**/
int8_t setModeContinuous (int8_t rate)
{
    return 1;
}





