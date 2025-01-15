// ESP32_ILI9341_openweathermap

// downloads json string from openweather.com
// for environmental conditions of (any) town
// displays alphanumerically, with gauges and with icons
//
// updates every five minutes in Loop -  check the variable named timerDelay
//
// microcontroller ESP32-WROOM-32
// current display TFT 018  =  320*240 ILI9341 controller
// implements Bodmer's TFT_ESPI library
// implements Bodmer's rainbow scale gauge
//
// json instructions by Rui Santos
// at https://RandomNerdTutorials.com/esp32-http-get-open-weather-map-thingspeak-arduino/
//
// public domain
// July 6, 2021 - minor issues fixed, compass pointer fixed
// Floris Wouterlood
//
// Make sure all the display driver and pin conections are correct by
// editing the User_Setup.h file in the TFT_eSPI library folder.
//
// ######################################################################################
// ######   DON'T FORGET TO UPDATE THE User_Setup.h FILE IN THE TFT_eSPI LIBRARY    #####
// ######################################################################################

#include <TFT_eSPI.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include "Free_Fonts.h"  // include the header file attached to this sketch - part of TFT_eSPI package

TFT_eSPI tft = TFT_eSPI();

#define WHITE 0xFFFF
#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define GREY 0x2108
#define SCALE0 0xC655      // accent color for unused scale segments
#define SCALE1 0x5DEE      // accent color for unused scale segments
#define TEXT_COLOR 0xFFFF  // is currently white

// rainbow scale ring meter color scheme
#define RED2RED 0
#define GREEN2GREEN 1
#define BLUE2BLUE 2
#define BLUE2RED 3
#define GREEN2RED 4
#define RED2GREEN 5

#define DEG2RAD 0.0174532925  // conversion factor degrees to radials

// data connection and acquisition variables
const char* ssid = "aintmynetwrk";                                     // network wifi credentials
const char* password = "random_password";                                 // wifi network key
String openWeatherMapApiKey = "getyourownkeyitsfree";  // API Openweathermap account
String city = "notreallyneeded";
String countryCode = "US";  // "NL" for The Netherlands
String jsonDocument(1024);  // memory allocation json string

// timer internet access for server download
unsigned long lastTime = 0;  // testing purposes = 30 seconds
unsigned long timerDelay;    // is set at 1 seconds in Setup and to 5 minutes in Loop

// joint variables
float temp_01 = 10;
float hum_01;
int hum_02;

// rainbow scale ring meter variables
uint32_t runTime = -99999;  // time for next update
int reading = 10;           // value to be displayed in circular scale
int tesmod = 0;
int rGaugePos_x = 0;                            // these two variables govern the position
int rGaugePos_y = 155;                          // of the square + gauge on the display
int ringmeterRadius = 65;                       // governs diameter of rainbow gauge
char* ringlabel[] = { "", "*F", "%", "mBar" };  // some custom labels
float tempRainbowgauge;
int t = 40;  // governs position of numerical output rainbow scale

// small needle meter
int j;
int pivotNeedle_x = 165;  // pivot coordinates needle of small gauge
int pivotNeedle_y = 222;
float center_x1 = 160;     // center x of edge markers circle left gauge
float center_y1 = 228;     // center y of edge markers circle left gauge
int radius_s = 65;         // for scale markers
int needleLength = 45;     // gauge needle length
int edgemarkerLength = 5;  // edge marker length
float edge_x1, edge_y1, edge_x1_out, edge_y1_out;
float angleNeedle = 0;
float needle_x, needle_y;
float needle_x_old, needle_y_old;
float angleCircle = 0;
int pivot_x = 165;  // pivot coordinates needle of small gauge
int pivot_y = 222;

// circle segment 'pie chart' meter variables
byte inc = 0;
unsigned int col = 0;
float pivotHumcircle__x = 198;
float pivotHumcircle__y = 274;
float startAngle = 0;
float subAngle;  // subtended angle
int r = 34;      // circle radius

// compass wind direction pointer
float compassPivot_x = 158;
float compassPivot_y = 65;
float c_x1, c_x2, c_x3, c_x4;
float c_y1, c_y2, c_y3, c_y4;
float c_x1_old, c_x2_old, c_x3_old, c_x4_old;
float c_y1_old, c_y2_old, c_y3_old, c_y4_old;

float windDir_01;
float compassAngle;
int compass_r = 22;
char* sector[] = { "N", "NE", "E", "SE", "S", "SW", "W", "NW", "N" };  // wind sector labels
int h;                                                                 // indicator for wind sector


void setup() {

  Serial.begin(115200);
  tft.init();
  tft.setRotation(2);
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  tft.println("network ");
  tft.println("connecting");


  WiFi.begin(ssid, password);
  Serial.println("connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    tft.print(".");
  }
  Serial.println();
  Serial.print("Connected to WiFi network ");
  tft.println("connected to ");
  tft.println();
  Serial.print(ssid);
  tft.println(ssid);
  delay(1000);
  tft.fillScreen(TFT_BLACK);

  Serial.print(" - IP adress: ");
  Serial.println(WiFi.localIP());
  Serial.println("timer set to 30 seconds (timerDelay variable) - it will take 30 seconds before publishing the first reading.");
  delay(1000);
  drawAllWindowFrames();  // instructions moved to subroutine
  tft.setTextColor(TFT_YELLOW, BLACK);

  tft.setFreeFont(FF1);

  tft.setTextSize(1);
  tft.setCursor(21, 20);
  tft.print("Pensacola - OpenWx");

  tft.setCursor(9, 45);
  tft.print("temp:");
  tft.setCursor(10, 67);
  tft.print(" bp :");
  tft.setCursor(10, 89);
  tft.print(" hum:");
  tft.setCursor(10, 111);
  tft.print("wind:");
  tft.setCursor(10, 133);
  tft.print("from:");

  tft.setFreeFont(FF0);  // note - units in Free Font 0 = text size 1
  tft.setCursor(133, 37);
  tft.print(char(247));  // degree character
  tft.print("F");
  tft.setCursor(133, 59);
  tft.print("mB");
  tft.setCursor(133, 81);
  tft.print("%");
  tft.setCursor(133, 102);
  tft.print("mph");

  tft.setCursor(10, 160);
  tft.print("temp");  // in rainbow scale

  tft.setCursor(125, 160);  // left upper corner = anchoring point of rainbow gauge scale
  tft.print(char(247));     // degree character
  tft.setTextSize(2);
  tft.print("F");
  tft.setTextSize(1);

  tft.setCursor(55, 290);  // 0  is bottom rainbow scale temperature
  tft.print("0");
  tft.setCursor(98, 290);  // 50 is top rainbow scale temperature
  tft.print("100");



  tft.setCursor(220, 160);  // display percent sign in  small gauge scale
  tft.print("%");

  drawSaleSmallGauge();
  tft.fillCircle(pivot_x, pivot_y, 2, MAGENTA);   // pivot needle middle small gauge                                                                         // arbitrary seeding temp - avoids drawing black line from position 0.0
  needleMeter();                                  // calculate position and draw the needle
  tft.drawRoundRect(158, 150, 80, 80, 4, GREEN);  // correction right middle small frame from buildup needle
  tft.setCursor(216, 217);
  tft.setTextSize(1);
  tft.print("40");  // scale starts with 40% and ends with 100%
  tft.setCursor(164, 169);
  tft.print("100");
  tft.setTextSize(2);

  // wind sector window on top of compass
  tft.setCursor(171, 51);
  tft.setTextSize(1);
  tft.print("wind:");

  tft.setCursor(170, 33);
  tft.setTextSize(1);
  tft.setTextColor(TFT_GREEN);
  tft.print("v1.1 - JPM");
  tft.setTextColor(TFT_WHITE);

  timerDelay = 1000;  // this is for fast initial query OpenWeather server for json string
  doTheHardWork();    // run this here - in void loop the timerDelay refreshes every 15 minutes
  drawAllWindowFrames();  // drawover to correct for all drawing errors caused by needles and markers
  rainbowScaleMeter();    // prepare the rainbow scale meter
  needleMeter();          // prepare the middle right, small meter with the needle
  circleSegmentMeter();   // prepare the circle segment pie meter
  compassGauge();         // prepare the wind compass meter
  compassPointer();       // add pointer to the wind compass meter
  windSectorReporter();   // add wind sector reporter

  //  Serial.println ("Setup complete");
}

void loop() {

  timerDelay = 900000;  // connect to OpenWeather and refresh every 15 minutes
  doTheHardWork();
}


// ######################################################################################
// #               functions section                                                    #
// ######################################################################################


void doTheHardWork() {
  //Serial.println ("Start hard work...");           // buncha prints for troubleshooting.
  // Send an HTTP GET request                        // guy started out strong but fumbled
  if ((millis() - lastTime) > timerDelay) {          // the functions, no return statements.
    // Check WiFi connection status                  // so program had no return from function.  
    if (WiFi.status() == WL_CONNECTED) {             // changed to voids and also v3.0 for owm.
      String serverPath = "https://api.openweathermap.org/data/3.0/onecall?lat=0.0&lon=0.0&exclude=minutely,hourly,daily&units=imperial&appid=yourownkeygoeshere";
      jsonDocument = httpGETRequest(serverPath.c_str());
      JSONVar myObject = JSON.parse(jsonDocument);  // JSON.typeof(jsonVar) can be used to get the type of the var
      Serial.println(JSON.typeof(myObject));        // prints: object
      double temp_01 = (myObject["current"]["temp"]);
      double hum_01 = (myObject["current"]["humidity"]);

      if (JSON.typeof(myObject) == "undefined") {
        Serial.println("Parsing input failed!");
        return;
      }

      Serial.println("*******************************************");
      Serial.print("JSON object = ");
      Serial.println(myObject);
      Serial.println("*******************************************");

      Serial.println("extracted from JSON object:");
      Serial.print("Temperature:    ");
      Serial.print(myObject["current"]["temp"]);
      Serial.println(" °F");
      Serial.print("Pressure:       ");
      Serial.print(myObject["current"]["pressure"]);
      Serial.println("inHg");
      Serial.print("Humidity:       ");
      Serial.print(myObject["current"]["humidity"]);
      Serial.println(" %");
      Serial.print("Wind Speed:     ");
      Serial.print(myObject["current"]["wind_speed"]);
      Serial.println(" mph");
      Serial.print("Wind Direction: ");
      Serial.print(myObject["current"]["wind_deg"]);
      Serial.println(" degrees");

      double windDir = (myObject["current"]["wind_deg"]);
      windDir_01 = windDir;

      compassGauge();
      compassPointer();
      windSectorReporter();

      // print some json data in upper window of the display

      //temp_01 = temp_01;                                 // convert temp from Kelvin to Celsius
      tempRainbowgauge = temp_01;
      hum_02 = hum_01;

      Serial.print("Wind from:      ");
      Serial.println(sector[h]);

      Serial.print("Rainbowgauge:   ");
      Serial.print(tempRainbowgauge, 1);
      Serial.println(" °F");
      Serial.println("");

      tft.setFreeFont(FF1);  // start display data representation

      tft.fillRect(60, 35, 70, 15, BLACK);
      tft.setCursor(72, 45);
      tft.print(temp_01, 1);

      tft.fillRect(60, 55, 70, 15, BLACK);
      tft.setCursor(72, 67);  // was 67
      tft.print(myObject["current"]["pressure"]);

      tft.fillRect(60, 77, 70, 15, BLACK);
      tft.setCursor(72, 89);  // was 75
      tft.print(myObject["current"]["humidity"]);

      tft.fillRect(60, 99, 70, 15, BLACK);
      tft.setCursor(72, 110);  // was 69
      int ws = (myObject["current"]["wind_speed"]);
      tft.print(ws);

      tft.fillRect(65, 121, 40, 15, BLACK);
      tft.setCursor(72, 133);  // was 78
      tft.print(sector[h]);    // wind sector

      tft.setFreeFont(FF0);  // end display numerical data representation
      rainbowScaleMeter();
      needleMeter();         // calculate needle position middle right, small meter
      circleSegmentMeter();  // calculate and display the pie chart
                             //   Serial.println ("circle segment complete");
    } else {
      Serial.println("WiFi Disconnected");
    }
    lastTime = millis();
  }  //Serial.println ("...hard work complete");
}

// ######################################################################################
// #  draw window frames                                                                #
// ######################################################################################

void drawAllWindowFrames() {
  //Serial.print( "Begin drawAllWindowFrames");
  tft.drawRoundRect(4, 2, 234, 25, 4, GREEN);     // title frame
  tft.drawRoundRect(4, 28, 150, 118, 4, GREEN);   // left upper numerical window
  tft.drawRoundRect(4, 150, 150, 165, 4, GREEN);  // left lower (big) frame for rainbow scale gauge
  tft.drawRoundRect(158, 150, 80, 80, 4, GREEN);  // right middle small frame - needle meter
  tft.drawRoundRect(158, 235, 80, 80, 4, GREEN);
  tft.drawRoundRect(158, 65, 80, 81, 4, GREEN);  // compass frame
  tft.drawRoundRect(158, 47, 80, 16, 4, GREEN);  // wind sector indicator frame
  tft.drawRoundRect(158, 28, 80, 16, 4, GREEN);  // version window
  // Serial.println( "...End drawAllWindowFrames");
}


// ######################################################################################
// #  HTTP GET sequence                                                                 #
// ######################################################################################

String httpGETRequest(const char* serverName) {
  //Serial.println( "Begin httpGETRequest");
  HTTPClient http;
  http.begin(serverName);             // your IP address with path or Domain name with URL path
  int httpResponseCode = http.GET();  // send HTTP POST request
  String payload = "{}";
  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  http.end();  // free microcontroller resources
               //  Serial.println( "...End httpGETRequest");
  return payload;
}


// ######################################################################################
// #  rainbow scale meter                                                               #
// ######################################################################################

void rainbowScaleMeter() {
  //Serial.print("Begin rainbowScaleMeter   ");
  if (millis() - runTime >= 100)  // originally 500 = delay
  {
    runTime = millis();
    if (tesmod == 0) {
      reading = 99;
    }
    if (tesmod == 1) {
      reading = tempRainbowgauge * 2;  // important: here ring is seeded with value
    }
    // Serial.print (reading);
    int xpos = 10, ypos = 240, gap = 100;  // position of upper ring and proportion

    ringMeter(reading, 0, 100, (rGaugePos_x + 15), (rGaugePos_y + 17), ringmeterRadius, ringlabel[0], GREEN2RED);
    tesmod = 1;
    //       Serial.print ("   tesmod = "); Serial.print(tesmod);
  }
  //  Serial.println ("...End rainbowScaleMeter");
}


// ######################################################################################
// #  rainbow scale: draw the rainbox ring meter, returns x coord of righthand side     #
// ######################################################################################

void ringMeter(int value, int vmin, int vmax, int x, int y, int r, char* units, byte scheme) {
  // Minimum value of r is about 52 before value text intrudes on ring
  // drawing the text first is an option
  //Serial.println (" Begin ringMeter");
  x += r;
  y += r;                                         // calculate coordinates of center of ring
  int w = r / 3;                                  // width of outer ring is 1/4 of radius
  int angle = 150;                                // half the sweep angle of the meter (300 degrees)
  int v = map(value, vmin, vmax, -angle, angle);  // map the value to an angle v
  byte seg = 3;                                   // segments are 3 degrees wide = 100 segments for 300 degrees
  byte inc = 6;                                   // draw segments every 3 degrees, increase to 6 for segmented ring
  int colour = BLUE;                              // variable to save "value" text color from scheme and set default


  for (int i = -angle + inc / 2; i < angle - inc / 2; i += inc)  // draw color blocks every increment degrees
  {
    float sx = cos((i - 90) * DEG2RAD);  // calculate pair of coordinates for segment start
    float sy = sin((i - 90) * DEG2RAD);
    uint16_t x0 = sx * (r - w) + x;
    uint16_t y0 = sy * (r - w) + y;
    uint16_t x1 = sx * r + x;
    uint16_t y1 = sy * r + y;

    float sx2 = cos((i + seg - 90) * DEG2RAD);  // calculate pair of coordinates for segment end
    float sy2 = sin((i + seg - 90) * DEG2RAD);
    int x2 = sx2 * (r - w) + x;
    int y2 = sy2 * (r - w) + y;
    int x3 = sx2 * r + x;
    int y3 = sy2 * r + y;

    if (i < v) {  // fill in coloured segments with 2 triangles
      switch (scheme) {
        case 0: colour = RED; break;                                      // fixed color
        case 1: colour = GREEN; break;                                    // fixed color
        case 2: colour = BLUE; break;                                     // fixed colour
        case 3: colour = rainbow(map(i, -angle, angle, 0, 127)); break;   // full spectrum blue to red
        case 4: colour = rainbow(map(i, -angle, angle, 70, 127)); break;  // green to red (high temperature etc)
        case 5: colour = rainbow(map(i, -angle, angle, 127, 63)); break;  // red to green (low battery etc)
        default: colour = BLUE; break;                                    // fixed color
      }
      tft.fillTriangle(x0, y0, x1, y1, x2, y2, colour);
      tft.fillTriangle(x1, y1, x2, y2, x3, y3, colour);
    } else  // fill in blank segments
    {
      tft.fillTriangle(x0, y0, x1, y1, x2, y2, SCALE1);  // color of the unoccupied ring scale
      tft.fillTriangle(x1, y1, x2, y2, x3, y3, SCALE0);  // color of the unoccupied ring scale
    }
  }

  tft.fillRect(t, y, 72, 15, BLACK);
  tft.setTextSize(2);
  if (tempRainbowgauge < -9.9) t = 33;
  if (tempRainbowgauge > -9.9) t = 38;
  if (tempRainbowgauge > 0) t = 50;
  if (tempRainbowgauge > 9.9) t = 42;


  tft.setCursor(t + 15, y);
  tft.setTextColor(GREEN);
  tft.print(tempRainbowgauge, 1);
  //Serial.println ("...ringMeter sub complete");
}


// ######################################################################################
// #  rainbow scale: 16-bit rainbow color mixer                                         #
// ######################################################################################

unsigned int rainbow(byte value) {  // value is expected to be in range 0-127
                                    //Serial.print( "Begin rainbow");                                                          // value is converted to a spectrum color from 0 = blue through to 127 = red
  byte red = 0;                     // red is the top 5 bits of a 16 bit colour value
  byte green = 0;                   // green is the middle 6 bits
  byte blue = 0;                    // blue is the bottom 5 bits
  byte quadrant = value / 32;

  if (quadrant == 0) {
    blue = 31;
    green = 2 * (value % 32);
    red = 0;
  }
  if (quadrant == 1) {
    blue = 31 - (value % 32);
    green = 63;
    red = 0;
  }
  if (quadrant == 2) {
    blue = 0;
    green = 63;
    red = value % 32;
  }
  if (quadrant == 3) {
    blue = 0;
    green = 63 - 2 * (value % 32);
    red = 31;
  }
  //Serial.println("...End rainbow");
  return (red << 11) + (green << 5) + blue;
}


// ######################################################################################
// # rainbox scale: return a value in range -1 to +1 for a given phase angle (degrees)  #
// ######################################################################################

float sineWave(int phase) {
  //Serial.print("Begin sinewave");
  return sin(phase * 0.0174532925);
  //Serial.println( "...End sinewave");
}


// ######################################################################################
// #  small needle meter       - scale creation                                         #
// ######################################################################################

void drawSaleSmallGauge() {
  //Serial.print( "Begin drawSaleSmallGauge");
  j = 270;  // start point of cirle segment
  do {
    angleCircle = (j * DEG2RAD);  // angle expressed in radians - 1 degree = 0,01745331 radians

    edge_x1 = (center_x1 + 4 + (radius_s * cos(angleCircle)));  // scale - note the 4 pixels offset in x
    edge_y1 = (center_y1 + (radius_s * sin(angleCircle)));      // scale

    edge_x1_out = (center_x1 + 4 + ((radius_s + edgemarkerLength) * cos(angleCircle)));  // scale - note the 4 pixels offset in x
    edge_y1_out = (center_y1 + ((radius_s + edgemarkerLength) * sin(angleCircle)));      // scale

    tft.drawLine(edge_x1, edge_y1, edge_x1_out, edge_y1_out, MAGENTA);

    j = j + 6;
  } while (j < 356);  // end of circle segment
  //Serial.println( "...End drawSaleSmallGauge");
}


// ######################################################################################
// #  small needle meter       - dynamic needle part                                    #
// ######################################################################################

void needleMeter() {
  //Serial.print( "Begin needleMeter  ");
  tft.drawLine(pivotNeedle_x, pivotNeedle_y, needle_x_old, needle_y_old, 0);  // remove old needle by overwritig in white

  angleNeedle = (420 * DEG2RAD - 1.5 * hum_02 * DEG2RAD);  // contains a 1.5 stretch factor to expand 60 percentage points over 90 degrees of scale

  if (angleNeedle > 6.28) angleNeedle = 6.28;                      // prevents the needle from ducking below horizontal
  needle_x = (pivotNeedle_x + ((needleLength)*cos(angleNeedle)));  // calculate x coordinate needle point
  needle_y = (pivotNeedle_y + ((needleLength)*sin(angleNeedle)));  // calculate y coordinate needle point
  needle_x_old = needle_x;                                         // remember previous needle position
  needle_y_old = needle_y;

  tft.drawLine(pivotNeedle_x, pivotNeedle_y, needle_x, needle_y, MAGENTA);
  tft.fillCircle(pivotNeedle_x, pivotNeedle_y, 2, MAGENTA);  // restore needle pivot
  //Serial.println ("...End needle meter");
}


// ######################################################################################
// #  circle segment meter - main body                                                  #
// ######################################################################################

void circleSegmentMeter() {
  //Serial.print( "Begin circleSegmentMeter");
  tft.fillCircle(pivotHumcircle__x, pivotHumcircle__y, 34, BLACK);
  fillSegment(pivotHumcircle__x, pivotHumcircle__y, 0, (360 * hum_02 / 100), 34, TFT_BLUE);  // draw pie chart segment
  tft.drawCircle(pivotHumcircle__x, pivotHumcircle__y, 35, YELLOW);                          // correct for faulty pixels circle edge

  tft.setCursor(pivotHumcircle__x + 10, pivotHumcircle__y - 5);
  tft.setTextColor(TFT_YELLOW);
  tft.setTextSize(1);
  tft.print("hum");
  tft.setCursor(pivotHumcircle__x + 10, pivotHumcircle__y + 5);
  tft.print(hum_02);
  tft.print("%");
  tft.setTextSize(2);
  //Serial.println( "...End circleSegmentMeter");
}


// ######################################################################################
// #  circle segment meter - draw segments                                              #
// ######################################################################################

// x,y == coords of centre of circle
// startAngle = 0 - 359
// subAngle   = 0 - 360 = subtended angle
// r = radius
// colour = 16 bit colour value

void fillSegment(int x, int y, int startAngle, int subAngle, int r, unsigned int colour) {
  //Serial.print( "Begin fillSegment");
  float sx = cos((startAngle - 90) * DEG2RAD);                                              // calculate first pair of coordinates for segment start
  float sy = sin((startAngle - 90) * DEG2RAD);
  uint16_t x1 = sx * r + x;
  uint16_t y1 = sy * r + y;

  for (int i = startAngle; i < startAngle + subAngle; i++)                                  // draw color blocks every inc degrees 
     {    
     int x2 = cos((i + 1 - 90) * DEG2RAD) * r + x;                                          // calculate pair of coordinates for segment end
     int y2 = sin((i + 1 - 90) * DEG2RAD) * r + y;
     tft.fillTriangle(x1, y1, x2, y2, x, y, colour);
     x1 = x2;                                                                               // copy segment end to segment start for next segment
     y1 = y2;
     }
  //Serial.println( "...End fillSegmyent");
}


// ######################################################################################
// #  constructs the compass gauge     - static part of compass                         #
// ######################################################################################

void compassGauge() {
  //Serial.print( "Begin compassGauge");
  tft.drawCircle((compassPivot_x + 40), (compassPivot_y + 40), compass_r + 11, BLUE);  // outer circle compass
  tft.drawCircle((compassPivot_x + 40), (compassPivot_y + 40), compass_r + 2, CYAN);   // inner circle compass - pointer fits in here
  tft.setTextSize(1);
  tft.setCursor(compassPivot_x + 38, compassPivot_y + 4);
  tft.setTextColor(WHITE, BLACK);
  tft.print("N");
  tft.setCursor(compassPivot_x + 38, compassPivot_y + 69);
  tft.print("S");
  tft.setCursor(compassPivot_x + 71, compassPivot_y + 35);
  tft.print("E");
  tft.setCursor(compassPivot_x + 5, compassPivot_y + 35);
  tft.print("W");
  //Serial.println( "...End compassGauge");
}


// ######################################################################################
// #  constructs the compass pointer     - dynamic part of compass                      #
// ######################################################################################

void compassPointer() {
  //Serial.print("Begin compassPointer");
  tft.fillTriangle(c_x1_old, c_y1_old, c_x2_old, c_y2_old, c_x4_old, c_y4_old, BLACK);  // remove old compass pointer by overwritig in white
  tft.fillTriangle(c_x1_old, c_y1_old, c_x3_old, c_y3_old, c_x4_old, c_y4_old, BLACK);  // remove old compass pointer by overwritig in white

  compassAngle = ((windDir_01 - 90) * DEG2RAD);

  c_x1 = ((compassPivot_x + 40) + (compass_r * cos(compassAngle)));  // calculate coordinates compass pointer tip
  c_y1 = ((compassPivot_y + 40) + (compass_r * sin(compassAngle)));

  c_x2 = ((compassPivot_x + 40) + (compass_r * cos(compassAngle + 163 * DEG2RAD)));  // calculate coordinates compass pointer base point
  c_y2 = ((compassPivot_y + 40) + (compass_r * sin(compassAngle + 163 * DEG2RAD)));

  c_x3 = ((compassPivot_x + 40) + (compass_r * cos(compassAngle - 163 * DEG2RAD)));  // calculate coordinates compass pointer base point
  c_y3 = ((compassPivot_y + 40) + (compass_r * sin(compassAngle - 163 * DEG2RAD)));

  c_x4 = ((compassPivot_x + 40) + ((compass_r - 4) * cos(compassAngle - 180 * DEG2RAD)));  // calculate coordinates compass pointer base point
  c_y4 = ((compassPivot_y + 40) + ((compass_r - 4) * sin(compassAngle - 180 * DEG2RAD)));

  c_x1_old = c_x1;
  c_x2_old = c_x2;
  c_x3_old = c_x3;
  c_x4_old = c_x4;
  c_y1_old = c_y1;
  c_y2_old = c_y2;
  c_y3_old = c_y3;
  c_y4_old = c_y4;

  tft.fillTriangle(c_x1, c_y1, c_x3, c_y3, c_x4, c_y4, 0x51F2);  // was 3A72 print the new pointer to display
  tft.fillTriangle(c_x1, c_y1, c_x2, c_y2, c_x4, c_y4, BLUE);    // print the new pointer to display
  //Serial.println( "...End compassPointer");
}


// ######################################################################################
// #  calculates wind sector                                                            #
// ######################################################################################

void windSectorReporter() {
  //Serial.print( "Begin windSectorReporter");
  h = 0;
  if (windDir_01 < 22.5) h = 0;
  if (windDir_01 > 22.5) h = 1;
  if (windDir_01 > 67.5) h = 2;
  if (windDir_01 > 112.5) h = 3;
  if (windDir_01 > 157.5) h = 4;
  if (windDir_01 > 202.5) h = 5;
  if (windDir_01 > 247.5) h = 6;
  if (windDir_01 > 292.5) h = 7;
  if (windDir_01 > 337.5) h = 8;

  tft.fillRect(208, 51, 16, 8, BLACK);

  tft.setCursor(210, 51);
  tft.print(sector[h]);
  //Serial.println( "...End windSectorReporter");
}
