// Program for reading 8 thermistors connected to a 4051 mux, address
// driven by 3 MCP23017 outputs in loop() function.  The mux output enters 
// analog0 with a 10k resistor tied to ground.  The thermistors are all 
// tied to the analog supply - assumed to be 5.0 volts.
//
// Returns a floating point number, .print(num, 1) to format
//
// ============================================================
// From 4051 data sheet input/output pin list, 11, 10, 9 (a0-a2)
//  (+5v)----- (Thermistor) --|................|
//                           13 14 15 12 1 5 2 4
//                                mux inputs
//                                     |
// (Ground) ---- (10k-Resistor) -------|
//                                     3
//                                 analog 0
//

void readAnalog()
{
   // clear the three analog mux address bits, poke in the mux address bits and write
   // back to the MCP23017, delay 2mS to let the ADC settle, read analog channel, then place
   // the raw ADC reading in the correct slot in the array.  We can convert to degrees later...
   byte state = expanderRead(exp26, GPIOA);
   // read the temperatures...
   for(int i = 0; i < 8; i++)
   {
      state &= 0b11111000;            // punch out the old address bits
      state |= i;                     // add in the new address bits, leave output
      writeByte(exp26, GPIOA, state); // same as it was except analog mux addr
      delay(2);
      analogArray[i] = analogRead(A0);
   }
}

float Thermistor(word RawADC) 
{ 
   float temp = log((10240000/RawADC) - 10000);
   temp = 1 / (0.001129148 + (0.000234125 * temp) + 
      (0.0000000876741 * temp * temp * temp));
   temp = temp - 273.15;                 // convert Kelvin to Celsius
   temp = (temp * 9.0)/ 5.0 + 32.0;      // then to F
   return temp;                          // Return the Temperature
}

void tempControl()
{// DC fan temperature controls... read status of present outputs
   byte state = expanderRead (exp26, GPIOB);
   if (Thermistor(analogArray[mainexhaustairtemp]) > (Thermistor(analogArray[ambientairtemp] + 2)))
   {  // turn on the first fan until it cools down
      bit_set(state, BIT(MainFan1));        // aux fan1 on top of box
   }
   else if (Thermistor(analogArray[mainexhaustairtemp]) < (Thermistor(analogArray[ambientairtemp] + 1)))
   {  // shut off the first fan after it cools down
      bit_clear(state, BIT(MainFan1));      // aux fan1 on top of box
   }

   if (Thermistor(analogArray[mainexhaustairtemp]) > (Thermistor(analogArray[ambientairtemp] + 3)))
   {  // turn on the second fan until it cools down
      bit_set(state, BIT(MainFan2));        // aux fan2 on top of box
   }
   else if (Thermistor(analogArray[mainexhaustairtemp]) < (Thermistor(analogArray[ambientairtemp] + 2)))
   {  // shut off the second fan after it cools down
      bit_clear(state, BIT(MainFan2));      // aux fan2 on top of box
   }

   if (Thermistor(analogArray[mainexhaustairtemp]) > (Thermistor(analogArray[ambientairtemp] + 4)))
   {  // turn on the third fan until it cools down
      bit_set(state, BIT(MainFan3));        // aux fan3 on top of box
   }
   else if (Thermistor(analogArray[mainexhaustairtemp]) < (Thermistor(analogArray[ambientairtemp] + 3)))
   {  // shut off the third fan after it cools down
      bit_clear(state, BIT(MainFan3));      // aux fan3 on top of box
   }

   if (Thermistor(analogArray[mainexhaustairtemp]) > (Thermistor(analogArray[ambientairtemp] + 5)))
   {  // turn on the fourth fan until it cools down
      bit_set(state, BIT(MainFan4));        // aux fan4 on top of box
   }
   else if (Thermistor(analogArray[mainexhaustairtemp]) < (Thermistor(analogArray[ambientairtemp] + 4)))
   {  // shut off the fourth fan after it cools down
      bit_clear(state, BIT(MainFan4));      // aux fan4 on top of box
   }
   writeByte(exp26, GPIOB, state);          // update state of the outputs
}


