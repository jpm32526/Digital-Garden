/** WoodBox_No_LCD.ino
 *
 * This program is designed to control the lights and fans for a grow box.
 * Control I/O is routed thru a MCP23017 I2C expander chip.  The RTC
 * is a DS1307, also I2C accessed. The outputs can be manually switched
 * as needed for maintenance purposes.  The RTC is set by the radio at startup
 * and usually every minute.  On/Off times updated on startup and at least
 * once a day at midnight.
 * See the Schematics below for wiring of the system.
 * Remapped I/O for distributed outputs 03/26/12
 *
 * TODO: Switching times to be controlled by radio to run 12, 18, or 24 hrs per day.
 *       Clean up pH calibration routines
 *       
 *                         System Schematic
 *
 *  -----------------------[ ATMega328 Pinout ]---------------------------
 *                             +---\/---+
 *  Reset                     1|        |28  PC5 (ADC5 / D 19) SCL
 *  Serial    RXD  (D 0) PD0  2|        |27  PC4 (ADC4 / D 18) SDA
 *  Serial    TXD  (D 1) PD1  3|        |26  PC3 (ADC3 / D 17)
 *  Radio IRQ INT0 (D 2) PD2  4|        |25  PC2 (ADC2 / D 16)
 *            INT1 (D 3) PD3  5|        |24  PC1 (ADC1 / D 15) pH amp in
 *                 (D 4) PD4  6|        |23  PC0 (ADC0 / D 14) Analog mux in
 *                      +Vcc  7|        |22  A_Gnd             Analog Gnd
 *                       Gnd  8|        |21  A_Ref             Analog Ref
 *               Crystal PB6  9|        |20  A_Vcc             Analog +Vcc
 *               Crystal PB7 10|        |19  PB5 (D 13)  SCK   to radio
 *                 (D 5) PD5 11|        |18  PB4 (D 12)  MISO  to radio
 *                 (D 6) PD6 12|        |17  PB3 (D 11)  MOSI  to radio
 *                 (D 7) PD7 13|        |16  PB2 (D 10)  SS    to radio
 *                 (D 8) PB0 14|        |15  PB1 (D  9)        greenLED
 *                             +--------+
 */
//
//  -----[ library code: ]------------------------------------------------
//
#define DEBUG                 1     // zero to turn off serial output
#define RF69_COMPAT           0     // set to 1 to use RFM69CW

#include <Wire.h>                   // I2C library
#include <RTClib.h>                 // for Real Time Clock
#include <JeeLib.h>                 // radio stuff
#include <Arduino.h>                // needs to be enabled
#include <EEPROM.h>                 // needs to be enabled
//
//  -----[ Data Package Structure ]---------------------------------------
//
typedef struct {
  int  pH, t_pwrs, t_mAir, t_mExhaust, t_mWater,
       t_cAir, t_cExhaust, t_cWater, t_Amb, do_state;
} PayloadTX; PayloadTX box;
//
//  -----[ Radio Parameters ]--------------------------------------
//
#define myNodeID              7     // RF12 node ID in the range 1-30
#define network             212     // RF12 Network group
#define freq        RF12_433MHZ     // Frequency of RFM12B module
const int NTP_node_id     = 30;     // NTP server id
//
//  -------------------------[ RTC pinout ]------------------------
//

#define DS1307     0x68             // DS1307 RTC is at I2C address 0x68
/*                             DS1307 RTC
 *                             +---\/---+
 * Crystal1                   1|        |8  +Vcc
 * Crystal2                   2|        |7  pulse output
 * CR-1220 Positive           3|        |6  SCL
 * Gnd                        4|        |5  SDA
 *                             +--------+

*/
#define exp26      0x26             // this MCP23017 is at I2C address 0x26
//
//  -----------------------[ MCP23017 pinout ]------------------
//
/*                              MCP23017
 *                             +---\/---+
 * Main Lamp             PB0  1|        |28  PA7 VegOrFlowerB
 * Clone Lamp            PB1  2|        |27  PA6 VegOrFlowerA
 * Air Pump              PB2  3|        |26  PA5 Not used
 * Water Heater          PB3  4|        |25  PA4 Not used
 * Main Fan 1            PB4  5|        |24  PA3 CloneFan
 * Main Fan 2            PB5  6|        |23  PA2 Analog mux A2
 * Main Fan 3            PB6  7|        |22  PA1 Analog mux A1
 * Main Fan 4            PB7  8|        |21  PA0 Analog mux A0
 * +Vcc                       9|        |20  INTA
 * Gnd                       10|        |19  INTB
 * No connection             11|        |18  /RESET High
 * SDA                       12|        |17  A2 High
 * SCL                       13|        |16  A1 High
 * No connection             14|        |15  A0 Low
 *                             +--------+
*/
// MCP23017 register pairs (everything except direction defaults to 0)
#define IODIRA     0x00   // IO direction
#define IODIRB     0x01   //    (0 = output, 1 = input (Default))
#define IPOLA      0x02   // Input polarity
#define IPOLB      0x03   //    (0 = normal, 1 = inverse)
#define GPINTENA   0x04   // Interrupt on change
#define GPINTENB   0x05   //    (0 = disable, 1 = enable)
#define DEFVALA    0x06   // Default comparison for interrupt on change
#define DEFVALB    0x07   //    (interrupts on opposite to mask)
#define INTCONA    0x08   // Interrupt control
#define INTCONB    0x09   //   (0 = interrupt on pin change, 1 = DEFVAL)
#define IOCON      0x0A   // IO Configuration: bank/mirror/seqop...etc
#define GPPUA      0x0C   // Pull-up resistor
#define GPPUB      0x0D   //   (0 = disabled, 1 = enabled)
#define INTFA      0x0E   // Interrupt flag (read only) : 
#define INTFB      0x0F   // (0 = no interrupt, 1 = pin caused interrupt)
#define INTCAPA    0x10   // Interrupt capture (read only) : 
#define INTCAPB    0x11   //   value of GPIO at time of last interrupt
#define GPIOA      0x12   // Port value. Write to change, read to obtain value
#define GPIOB      0x13
#define OLATA      0x14   // Output latch. Write to latch output.
#define OLATB      0x15

//  pH EEPROM calibration parameter addresses
#define pH_INPUT 1        // analog input      
#define CALIBRATION_SOLUTION_1_MEM 1
#define CALIBRATION_SOLUTION_2_MEM 2
#define CALIBRATION_VALUE_1_MEM 3  // 2 bytes for integer
#define CALIBRATION_VALUE_2_MEM 5  // 2 bytes for integer

// exp26 portB MCP23017 output pins...
const byte MainLight        =  0;   // drives AC relay control pins
const byte CloneLight       =  1;   // via MCP23017 pins 1-4
const byte AirPump          =  2;   // on exp26 portB
const byte MainHeater       =  3;   //
const byte MainFan1         =  4;   // drives DC relay control pins
const byte MainFan2         =  5;   // via MCP23017 pins 5-8
const byte MainFan3         =  6;   // on exp26 portB
const byte MainFan4         =  7;   //
const byte CloneFan         =  3;   // on exp26 portA
//  -----[ Analog In Definitions ]---------------------------------------
//
// define the analog inputs...
const byte powersupplytemp     = 7; // analog inputs to 4051. addresses are from
const byte mainexhaustairtemp  = 1; // MCP23017 pins, selected channel is routed to
const byte mainwatertemp       = 2; // A0 analog input for conversion, data is
const byte mainairtemp         = 3; // then stored in analogArray[] and recalled
const byte cloneexhaustairtemp = 4; // as needed for display or logging
const byte cloneairtemp        = 5; //
const byte clonewatertemp      = 6; //
const byte ambientairtemp      = 0; //
//
//  -----[ Digital I/O Definitions ]--------------------------------------
//
#define greenLED               9  // IC 15 - test output for debug

//
//  -----[ Bit Twiddle Macro Definitions ]---------------------------------
//
#define bit_get(p,m) ((p) & (m))    // if(bit_get(foo, BIT(3))){do something}
#define bit_set(p,m) ((p) |= (m))   // bit_set(foo, BIT(5));
#define bit_clear(p,m) ((p) &= ~(m))// bit_clear(foo, BIT(5));
#define bit_flip(p,m) ((p) ^= (m))  // bit_flip(foo, BIT(0));
// bit_write(bit_get(foo, BIT(4)), bar, BIT(0));
#define bit_write(c,p,m) (c ? bit_set(p,m) : bit_clear(p,m))
#define BIT(x) (0x01 << (x))
#define LONGBIT(x) ((unsigned long)0x00000001 << (x))
//
//  -----[ Global Variables ]------------------------------------------------
//
word analogArray[8]          ;      // analog inputs stored as raw ADC reading
int MainStartTime        = 18;      // default start time for main light while on Auto
int CloneStartTime       = 18;      // default start time for clone light while on Auto
int MainStopTime         =  6;      // default stop time for main light while on Auto
int CloneStopTime        =  6;      // default stop time for clone light while on Auto
int  packet              =  0;      // packet number
byte now[6]                  ;      // time array
boolean TIME_REFRESHED   =  0;      // Non-zero = time has been recently updated by radio
unsigned long fast_update;          // system update rate
unsigned long slow_update;          // transmit rate
//
//  -----[ Library Create Object ]--------------------------------
//
RTC_DS1307 RTC;
//
//  -----[ System Setup ]-----------------------------------------
//
void setup()
{
  pinMode (greenLED, OUTPUT);     // set D9 as a digital output
  digitalWrite(greenLED, HIGH);

  Wire.begin();
  RTC.begin();
  //  initialize the radio, see radio definitions
  rf12_initialize(myNodeID, freq, network);

#if DEBUG
  Serial.begin(115200);
  Serial.print(F("\n\nGrow Box Control Mega328 v.1 No LCD\nI think the current time is...."));
  PrintDateTime(RTC.now());           // print to serial during debug
  Serial.print(F("\nBut I'm waiting for time update...."));
#endif

  /*  -----[ Setup MCP23017 I/O Registers ]-----   */
  writeByte (exp26, IODIRA,   0xF0);  // bits 0-3 outputs this port
  writeByte (exp26, IODIRB,   0x00);  // all outputs this port
  writeByte (exp26, GPIOA,    0x00);  // all outputs this port off
  writeByte (exp26, GPIOB,    0x00);  // all outputs this port off
  writeByte (exp26, GPPUA,    0xF0);  // pull-up resistor for switches
  writeByte (exp26, IPOLA,    0xF0);  // invert signal - if input low, reads 1

  waitForTimeUpdate(NTP_node_id);     // make sure time is accurate
  updateStartStopTimes();             // make sure the S/S times are set properly

  AirPumpOn();                        // stays on, only manual switch to turn off

  EEPROM.write(CALIBRATION_SOLUTION_1_MEM, 7);
  EEPROM.write(CALIBRATION_SOLUTION_2_MEM, 4);

#if DEBUG
  PrintDateTime(RTC.now());           // print to serial during debug
  Serial.print(F("System Restart\t"));
  Serial.print(F("I/O setup complete"));
  delay(1000);
#endif

  digitalWrite(greenLED, LOW);        // setup complete
}
//
//  -----[ Main Program Loop ]-------------------------------------------
//
void  loop()
{ /*   loop function sequence....
       [1] check for time update while idle,
       [2] then every second read the analog inputs,
       [3] then read the timers,
       [4] then update the temperature controls,
       [5] then every minute send the data.
   */
  // step [1] listen for time update
  if (rf12_recvDone())
  {
    if (rf12_crc == 0 && (rf12_hdr & RF12_HDR_CTL) == 0)  // and no rf errors
    {
      int node_id = (rf12_hdr & 0x1F);
      if (node_id == NTP_node_id )    // NTP server
      {
        timeUpdate(node_id);
      }
    }
  }

  if ((millis() - fast_update) >= 1000)
  { // scan these subs every second
    fast_update = millis();
    readAnalog();     // [2] update the analog temperature array
    readTimers();     // [3] check to see if anything scheduled
    tempControl();    // [4] check to see if any temperature deviations
  }

  if ((millis() - slow_update) >= 60000)
  { // calculate the current temps, get output states and transmit
    slow_update = millis();
    digitalWrite(greenLED, HIGH);

    box.pH         = int(readpH() * 100);
    box.t_pwrs     = Thermistor(analogArray[powersupplytemp]) * 100;
    box.t_mAir     = Thermistor(analogArray[mainairtemp]) * 100;
    box.t_mExhaust = Thermistor(analogArray[mainexhaustairtemp]) * 100;
    box.t_mWater   = Thermistor(analogArray[mainwatertemp]) * 100;
    box.t_cAir     = Thermistor(analogArray[cloneairtemp]) * 100;
    box.t_cExhaust = Thermistor(analogArray[cloneexhaustairtemp]) * 100;
    box.t_cWater   = Thermistor(analogArray[clonewatertemp]) * 100;
    box.t_Amb      = Thermistor(analogArray[ambientairtemp]) * 100;
    box.do_state   = expanderRead (exp26, GPIOB);

    rf12_sendNow(0, &box, sizeof box);
    rf12_sendWait(0);
    delay(25);
//    /*
    #ifdef DEBUG
          PrintDateTime(RTC.now());
          Serial.print(F(" data sent "));      // log event
    #endif
//    */
    digitalWrite(greenLED, LOW);
  }
}

//
//  -----[ I2C read/write Functions ]------------------------------------
//
// general I2C write data to register, for example output byte
void writeByte (const byte address, const byte reg, const byte data )
{
  Wire.beginTransmission (address);
  Wire.write (reg);       // any data or control register
  Wire.write (data);      // an 8 bit data byte
  Wire.endTransmission ();
} // end of I2C writeByte

// read a byte from an I2C device
byte expanderRead (const byte address, const byte reg)
{
  Wire.beginTransmission (address);
  Wire.write (reg);
  Wire.endTransmission ();
  Wire.requestFrom (address, (byte) 1);
  return Wire.read();
} // end of I2C expanderRead

/*
 *
 * Manual control switches are of type on-on-on. Not your usual toggle sw.
 *
 *                     Switch wiring
 *                        +5 --o o
 *                              /----jumper
 *                             o o --Output + LED to gnd
 *
 *                      Auto --o o --gnd
 *
 *
 *                  o o     o o     o o
 *                  |/|      /|      /
 *                  o o     o o     o o
 *                          |       | |
 *                  o o     o o     o o
 *           Toggle down   center   up
 *               man on     auto  man off
 */







