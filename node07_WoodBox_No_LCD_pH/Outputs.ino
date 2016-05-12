/*
 * This section controls the relays which controls the various
 * output devices.  The functions will turn off or on the device
 * based on the start/stop times set up in the <Timers> tab.
 * The functions will also log the last operation to the serial
 * port.  I/O is thru the MPC23017 devices, exp26.
 *
 *
 */
void MainLightOn()
{
   byte state = expanderRead(exp26, GPIOB);  // read output status
   if(!(bit_get(state, BIT(MainLight))))     // bail if already set
   {  // if not already set...
      bit_set(state, BIT(MainLight));        // set the bit HIGH (on)
      writeByte (exp26, GPIOB, state);       // write to the expander
#if DEBUG
      PrintDateTime(RTC.now());              // log event
      Serial.print(F("Main Light on"));
#endif
   }
}

void MainLightOff()
{
   byte state = expanderRead(exp26, GPIOB);  // read output status
   if(bit_get(state, BIT(MainLight)))        // bail if already cleared
   {  // if already set, turn off...
      bit_clear(state, BIT(MainLight));      // set the bit LOW (off)
      writeByte (exp26, GPIOB, state);       // write to the expander
#if DEBUG
      PrintDateTime(RTC.now());              // log event
      Serial.print(F("Main Light off"));
#endif
   }
}

void CloneLightOn()
{
   byte state = expanderRead(exp26, GPIOB);  // read output status
   if(!(bit_get(state, BIT(CloneLight))))    // bail if already set
   {  // if not already set...
      bit_set(state, BIT(CloneLight));       // set the bit HIGH (on)
      writeByte (exp26, GPIOB, state);       // write to the expander
      state = expanderRead(exp26, GPIOA);    // read output status
      bit_set(state, BIT(CloneFan));         // and set the clone fan on
      writeByte (exp26, GPIOA, state);       // write to the expander
#if DEBUG
      PrintDateTime(RTC.now());              // log event
      Serial.print(F("Clone Light on"));
#endif
   }
}

void CloneLightOff()
{
   byte state = expanderRead(exp26, GPIOB);  // read output status
   if(bit_get(state, BIT(CloneLight)))       // bail if already cleared
   {  // if already set, turn off...
      bit_clear(state, BIT(CloneLight));     // set the bit LOW (off)
      writeByte (exp26, GPIOB, state);       // write to the expander
      state = expanderRead(exp26, GPIOA);    // read output status
      bit_clear(state, BIT(CloneFan));       // and turn off the fan too
      writeByte (exp26, GPIOA, state);       // write to the expander
#if DEBUG
      PrintDateTime(RTC.now());              // log event
      Serial.print(F("Clone Light off"));
#endif
   }
}

void AirPumpOn()
{
   byte state = expanderRead(exp26, GPIOB);  // read output status
   if(!(bit_get(state, BIT(AirPump))))       // bail if already set
   {  // if not already set...
      bit_set(state, BIT(AirPump));          // set the bit HIGH (on)
      writeByte (exp26, GPIOB, state);       // write to the expander
#if DEBUG
      PrintDateTime(RTC.now());              // log event
      Serial.print(F("Air Pump on"));
#endif
   }
}

/*
void AirPumpOff()
{
   byte state = expanderRead(exp26, GPIOB);  // read output status
   if(bit_get(state, BIT(AirPump)))          // bail if already cleared
   {  // if already set, turn off...
      bit_clear(state, BIT(AirPump));        // set the bit LOW (off)
      writeByte (exp26, GPIOB, state);       // write to the expander
#if DEBUG
      PrintDateTime(RTC.now());              // log event
      Serial.print(F("Air Pump off"));
#endif
   }
}
*/


