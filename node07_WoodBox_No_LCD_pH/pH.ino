/*
 * 
 * This section of code is based on code from the PracticalMaker.
 * The website is no longer active, and it's a shame 'cause he had
 * some really good products and code.
 * 
 * Rewoking the code to use EEPROM.get and .put to fetch and store
 * a struct which contains the calibration values.
 * 
 * Calibration currently disabled while rework in progress.
 * 
 */

 
int highValue(int value)
{
  return value / 256;
}

int lowValue(int value)
{
  return value % 256;
}

int combineValue(unsigned int lb, unsigned int hb)
{
  return ((hb * 256) + lb);
}

void configure()
{
  int oldValue = 1;
  int newValue = 0;

  int matchCount = 0;
  byte forCounter = 0;
  long reading = 0;
  /*
    byte incomingByte;
    byte counter = 0;

    int serialData;

    while(Serial.available() == 0) {
      Serial.print(F("Please type in the value of the first calibration solution\n"));
      delay(5000);
    }
    if(Serial.available() > 0) {
      while(Serial.available() > 0) {
        incomingByte = Serial.read() - '0';
        if(counter == 0) {
          serialData = incomingByte;
          counter++;
        } else if(counter == 1) {
          serialData = (serialData*10) + incomingByte;
        }
      }
      counter = 0;
  */

#ifdef DEBUG
  Serial.print(F("Now configure for pH: "));
  Serial.println(EEPROM.read(CALIBRATION_SOLUTION_1_MEM));
  Serial.print(F("Please place the probe in solution now\n"));
  Serial.print(F("I will automatically calibrate this, but it may take some time\n"));
  Serial.print(F("I'll let you know when it's time to move on\n"));
#endif

  reading = 0;
  while (matchCount < 250) {
    oldValue = newValue;
    reading = 0;
#ifdef DEBUG
    Serial.print(F("Not stable, carrying on\n"));
#endif
    for (byte i = 0; i < 255; i++)
    {
      reading = reading + analogRead(pH_INPUT);
      delay(10);
    }
    newValue = reading / 255;

    if (newValue == oldValue)
    {
      matchCount++;
    }

#ifdef DEBUG
    Serial.print(F("Old value: "));
    Serial.print(oldValue);
    Serial.print(F("\tNew value: "));
    Serial.print(newValue);
    Serial.print(F("\tMatch Count: "));
    Serial.println(matchCount);
    delay(500);
#endif
  }
  matchCount = 0;
  EEPROM.write(CALIBRATION_VALUE_1_MEM, lowValue(newValue));
  EEPROM.write(CALIBRATION_VALUE_1_MEM + 1, highValue(newValue));
  oldValue = 1;
  newValue = 0;



  //second calibration solution
  /*
  while(Serial.available() == 0) {
      Serial.print(F("Please type in the value of the second calibration solution\n"));
      delay(5000);
    }
    if(Serial.available() > 0) {
      while(Serial.available() > 0) {
        incomingByte = Serial.read() - '0';
        if(counter == 0) {
          serialData = incomingByte;
          counter++;
        } else if(counter == 1) {
          serialData = (serialData*10) + incomingByte;
        }
      }
      counter = 0;
  */
#ifdef DEBUG
  Serial.print(F("Now configure for pH: "));
  Serial.print(EEPROM.read(CALIBRATION_SOLUTION_2_MEM));
  Serial.print(F("\nPlease place the probe in solution now"));
  Serial.print(F("\nI will automatically calibrate this, but it may take some time\n"));
  Serial.print(F("I'll let you know when it's time to move on\n"));
#endif

  reading = 0;
  while (matchCount < 250)
  {
    oldValue = newValue;
    reading = 0;
#ifdef DEBUG
    Serial.print(F("Not stable, carrying on\n"));
#endif

    reading = 0;
    for (byte i = 0; i < 255; i++)
    {
      reading = reading + analogRead(pH_INPUT);
      delay(10);
    }
    newValue = reading / 255;

    if (newValue == oldValue) matchCount++;

#ifdef DEBUG
    Serial.print(F("Old value: "));
    Serial.print(oldValue);
    Serial.print(F("   New value: "));
    Serial.print(newValue);
    Serial.print(F("   Match Count: "));
    Serial.println(matchCount);
    delay(500);
#endif
  }
  matchCount = 0;
  EEPROM.write(CALIBRATION_VALUE_2_MEM, lowValue(newValue));
  EEPROM.write(CALIBRATION_VALUE_2_MEM + 1, highValue(newValue));
}

float readpH() {
  int y1 = EEPROM.read(CALIBRATION_SOLUTION_1_MEM);
  int y2 = EEPROM.read(CALIBRATION_SOLUTION_2_MEM);
  int x1 = combineValue(EEPROM.read(CALIBRATION_VALUE_1_MEM), EEPROM.read(CALIBRATION_VALUE_1_MEM + 1));
  int x2 = combineValue(EEPROM.read(CALIBRATION_VALUE_2_MEM), EEPROM.read(CALIBRATION_VALUE_2_MEM + 1));
  //work out m for y = mx + b
  float m = ((float)y1 - (float)y2) / ((float)x1 - (float)x2);
  //work out b for y = mx +
  float b = (float)y1 - ((float)m * (float)x1);

/*
#ifdef DEBUG
  Serial.print(F("\nX1: "));
  Serial.print(x1);
  Serial.print(F("\tX2: "));
  Serial.print(x2);
  Serial.print(F("\nY1: "));
  Serial.print(y1);
  Serial.print(F("\tY2: "));
  Serial.print(y2);
  Serial.print(F("\nM: "));
  Serial.print(m);
  Serial.print(F("\tB: "));
  Serial.println(b);
#endif
*/
  long reading = 0;
  for (byte i = 0; i < 255; i++)
  {
    reading = reading + analogRead(pH_INPUT);
    delay(10);
  }
  reading = reading / 255;
/*#ifdef DEBUG
  Serial.print(F("Analog: "));
  Serial.println(reading);
#endif
*/
  float y = (m * (float)reading) + b;
  return y;
}

