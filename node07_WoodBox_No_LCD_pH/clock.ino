
//
//  -----[ format date/time string for serial output ]---------------------------------
// 
void PrintDateTime(DateTime t)
{
   char datestr[24];
   sprintf(datestr, "\n%02d:%02d:%02d  %02d-%02d-%04d\t", t.hour(), t.minute(), t.second(), t.month(), t.day(), t.year());
   Serial.print(datestr);  
}

//
//  -----[ wait for time update from NTP server ]---------------------------------
// 
void waitForTimeUpdate(int id)
{
   while (!TIME_REFRESHED)
   {
      digitalWrite(greenLED, HIGH);
      if (rf12_recvDone())
      { 
         if (rf12_crc == 0 && (rf12_hdr & RF12_HDR_CTL) == 0)  // no rf errors
         { 
            int node_id = (rf12_hdr & 0x1F);
            if (node_id == id )  // NTP server
            { 
               timeUpdate(id);
            }
            digitalWrite(greenLED, LOW);
         }
      }
   }  
}

//
//  -----[ received time update from NTP server ]---------------------------------
// 
void timeUpdate(int id)
{
   DateTime dt0 (rf12_data[7], rf12_data[6], rf12_data[5], rf12_data[1], rf12_data[2], rf12_data[3]);
   RTC.adjust(dt0);
   TIME_REFRESHED = true;
/*
#if DEBUG
   PrintDateTime(RTC.now());              // log event
   Serial.print(F(" time update from node "));
   Serial.print(id);
#endif
*/
}

