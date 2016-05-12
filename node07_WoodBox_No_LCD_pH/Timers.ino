void readTimers()
{ // set up lamp controls...
   // optimize power usage to avoid the high rate periods.
   // winter rates begin in Nov and run thru end of April.
   // lights, pumps, and fans schedule start/stop from here.

   // test normally at midnight or reset, default is winter rate period
   DateTime now = RTC.now();
   if((!now.hour()) && (!now.minute()) && (!now.second()))
   {  // ...it's midnight...
      digitalWrite(greenLED, HIGH);
      TIME_REFRESHED = 0;
      while (!TIME_REFRESHED)
      {
         digitalWrite(greenLED, HIGH);
         waitForTimeUpdate(NTP_node_id); // wait for NTP transmission
         digitalWrite(greenLED, LOW);

         updateStartStopTimes();
      }
   }
   // otherwise, from here down, test every second...
   if (now.hour() >= MainStartTime || now.hour() < MainStopTime) 
   {  // default 12 hours, 1700 winter, 1800 summer to 05 or 06 stop
      MainLightOn(); 
   }
   else
   {  // otherwise, shut off light
      MainLightOff();
   }

   if (now.hour() >= CloneStartTime || now.hour() < CloneStopTime) 
   {  // 18 hours, 1100 winter, 1800 summer to 05 or noon
      CloneLightOn();    
   }
   else
   {  // 18 hours on, 0500 winter, noon summer cutoff
      CloneLightOff(); 
   }
}

void updateStartStopTimes()
{  // set the run time for the main light.  exp26 bits 6 and 7
   // are read to determine if the start/stop times need to be
   // adjusted
   boolean rate = setRatePeriod(RTC.now());
   byte state = expanderRead (exp26, GPIOA) & 0xC0; // bit pattern 0b11000000
   switch (state)
   {  // cases 0x80 and 0x40 yield the same results - 18hr.
   case 0xC0: // set for 12hr main run - bit pattern 0b11000000
      // no changes needed
      break;                    // end 12hr main
   case 0x40: // set for 18hr main run - bit pattern 0b01000000
   case 0x80: // set for 18hr main run - bit pattern 0b10000000
      MainStopTime  += 6;       // add 6 hours to main stop, start stays same
      break;                    // end 18hr main
   case 0x00: // set for 24hr main run - bit pattern 0b00000000
      MainStartTime  =  0;      // start now...
      MainStopTime   = 25;      // ...and don't ever get to stop
      MainLightOn(); 
      break;                    // end set 24hr main
   }

#if DEBUG
   PrintDateTime(RTC.now());
   Serial.print(F("Main  On/Off "));
   Serial.print(MainStartTime);
   Serial.print('/');
   Serial.print(MainStopTime);
   Serial.print(F("\tClone On/Off "));
   Serial.print(CloneStartTime);
   Serial.print('/');
   Serial.print(CloneStopTime);
   Serial.print('\n');
#endif
}

bool setRatePeriod(DateTime t)
{   
   // default start/stop times depending on season
   if(t.month() > 10 || t.month() < 5)
   {
      MainStartTime  = 17;      // Winter rates
      MainStopTime   =  5;
      CloneStartTime = 11;
      CloneStopTime  =  5;
      return true;
   }
   else
   { 
      MainStartTime  = 18;      // Summer rates
      MainStopTime   =  6;      // morning off time
      CloneStartTime = 18;
      CloneStopTime  = 12;
      return false;
   }
}
