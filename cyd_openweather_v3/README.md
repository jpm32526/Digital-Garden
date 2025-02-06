# ESP32 ILI9341 OpenWeatherMap v3.0

## Overview

This code runs on an ESP32 microcontroller and downloads JSON data from OpenWeatherMap to display environmental conditions for a specified location. It uses a TFT display to show the information both alphanumerically and graphically with gauges and icons.

## Features

- **Microcontroller**: ESP32-WROOM-32
- **Display**: 320x240 ILI9341 TFT
- **Libraries**: 
  - Bodmer's TFT_ESPI
  - Bodmer's rainbow scale gauge
- **Data Source**: OpenWeatherMap API
- **Update Frequency**: Every 15 minutes (adjustable via `timerDelay` variable)

## Setup Instructions

1. **Edit User_Setup.h**: Ensure all display driver and pin connections are correct by editing the `User_Setup.h` file in the TFT_eSPI library folder.
2. **OpenWeatherMap Key**: Insert your OpenWeatherMap key and latitude/longitude values on line 264.

## Code Breakdown

### Libraries and Initialization

```cpp
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include "Free_Fonts.h"

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
```

### WiFi and OpenWeatherMap Configuration

```cpp
const char* ssid = "aintmynetwrk";
const char* password = "random_password";
String openWeatherMapApiKey = "getyourownkeyitsfree";
String city = "notreallyneeded";
String countryCode = "US";
String jsonDocument(1024);
```

### Timer Variables

```cpp
unsigned long lastTime = 0;
unsigned long timerDelay;
```

### Setup Function

```cpp
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

  Serial.print(" - IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("timer set to 30 seconds (timerDelay variable) - it will take 30 seconds before publishing the first reading.");
  delay(1000);
  drawAllWindowFrames();
  tft.setTextColor(TFT_YELLOW, BLACK);
  tft.setFreeFont(FF1);
  tft.setTextSize(1);
  tft.setCursor(21, 20);
  tft.print("Pensacola - OpenWx");

  // Display labels and units
  tft.setCursor(9, 45); tft.print("temp:");
  tft.setCursor(10, 67); tft.print(" bp :");
  tft.setCursor(10, 89); tft.print(" hum:");
  tft.setCursor(10, 111); tft.print("wind:");
  tft.setCursor(10, 133); tft.print("from:");

  tft.setFreeFont(FF0);
  tft.setCursor(133, 37); tft.print(char(247)); tft.print("F");
  tft.setCursor(133, 59); tft.print("mB");
  tft.setCursor(133, 81); tft.print("%");
  tft.setCursor(133, 102); tft.print("mph");

  tft.setCursor(10, 160); tft.print("temp");
  tft.setCursor(125, 160); tft.print(char(247)); tft.setTextSize(2); tft.print("F");
  tft.setTextSize(1);
  tft.setCursor(55, 290); tft.print("0");
  tft.setCursor(98, 290); tft.print("100");

  tft.setCursor(220, 160); tft.print("%");

  drawSaleSmallGauge();
  tft.fillCircle(pivot_x, pivot_y, 2, MAGENTA);
  needleMeter();
  tft.drawRoundRect(158, 150, 80, 80, 4, GREEN);
  tft.setCursor(216, 217); tft.setTextSize(1); tft.print("40");
  tft.setCursor(164, 169); tft.print("100");
  tft.setTextSize(2);

  tft.setCursor(171, 51); tft.setTextSize(1); tft.print("wind:");
  tft.setCursor(170, 33); tft.setTextSize(1); tft.setTextColor(TFT_GREEN); tft.print("v1.1 - JPM");
  tft.setTextColor(TFT_WHITE);

  timerDelay = 1000;
  doTheHardWork();
  drawAllWindowFrames();
  rainbowScaleMeter();
  needleMeter();
  circleSegmentMeter();
  compassGauge();
  compassPointer();
  windSectorReporter();
}
```

### Loop Function

```cpp
void loop() {
  timerDelay = 900000;  // Refresh every 15 minutes
  doTheHardWork();
}
```

### Function Definitions

- **doTheHardWork()**: Sends an HTTP GET request to the OpenWeatherMap API, parses the JSON response, and updates the display.

- **drawAllWindowFrames()**: Draws the frames for various display sections.

- **httpGETRequest()**: Handles the HTTP GET request and returns the response as a string.

- **rainbowScaleMeter()**: Updates the rainbow scale meter with the latest data.

- **ringMeter()**: Draws the rainbow ring meter.

- **rainbow()**: Generates a 16-bit rainbow color value.

- **drawSaleSmallGauge()**: Draws the scale for the small needle meter.

- **needleMeter()**: Updates the needle position for the small gauge.

- **circleSegmentMeter()**: Updates the circle segment meter.

- **fillSegment()**: Draws a segment of the circle meter.

- **compassGauge()**: Draws the static part of the compass gauge.

- **compassPointer()**: Updates the dynamic compass pointer.

- **windSectorReporter()**: Calculates and displays the wind sector.

## Important Notes

- Make sure to update the `User_Setup.h` file in the TFT_eSPI library with the correct display driver and pin connections if you're not using an ESP32 Cheap Yellow Display – CYD (ESP32-2432S028R).
- Insert your OpenWeatherMap API key and latitude/longitude values in the code where indicated.
- The code refreshes the data every 15 minutes by default, adjust the `timerDelay` variable as needed.
