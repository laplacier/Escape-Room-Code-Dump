/*
 * This program is designed for use with SN74HC165N  and SN74HC595N
 * shift registers.
 * 
 * PISO - Parallel in, Serial out. The SN74HC165N is a PISO
 * register. It accepts 8 inputs in parallel and outputs their 
 * states on one shiftable pin.
 * SIPO - Serial in, parallel out. The SN74HC595N is a SIPO 
 * register. It accepts 1 byte shifted in on one pin and outputs 
 * each bit to 8 parallel outputs.
 * 
 * The last PISO register in the chain will 
 * have nothing connected to pin 10, and the last SIPO register 
 * will have nothing connected to pin 9.
 * Each input pin of the PISO registers must be pulled high
 * with a resistor (recommended values between 1k-10k)
 * 
 * PISO#0 __ __                         SIPO#0 __ __
 *       |  U  |                              |  U  |
 *  LOAD-|1  16|-VCC                        B-|1  16|-VCC
 *   CLK-|2  15|-GND                        C-|2  15|-A
 *     E-|3  14|-D                          D-|3  14|-DATA
 *     F-|4  13|-C                          E-|4  13|-GND
 *     G-|5  12|-B                          F-|5  12|-LATCH
 *     H-|6  11|-A                          G-|6  11|-CLK
 *      -|7  10|- to PISO#1 pin 9           H-|7  10|-VCC
 *   GND-|8   9|-DATA                     GND-|8   9|- to SIPO#1 pin 14
 *       |_____|                              |_____|
 *      
 * PISO#1 __ __                         SIPO#1 __ __
 *       |  U  |                              |  U  |
 *  LOAD-|1  16|-VCC                       B'-|1  16|-VCC
 *   CLK-|2  15|-GND                       C'-|2  15|-A'
 *    E'-|3  14|-D'                        D'-|3  14|- to SIPO#0 pin 9
 *    F'-|4  13|-C'                        E'-|4  13|-GND
 *    G'-|5  12|-B'                        F'-|5  12|-LATCH
 *    H'-|6  11|-A'                        G'-|6  11|-CLK
 *      -|7  10|- to PISO#2 pin 9, etc...  H'-|7  10|-VCC
 *   GND-|8   9|- to PISO#0 pin 10        GND-|8   9|- to SIPO#2 pin 14, etc...
 *       |_____|                              |_____|
 */

/* 
 * Function: isDataNew
 * Checks all PISO register states, returns true
 * when a change is detected.
 */
bool isDataNew(){
  for(int i=0; i<numPISO; i++){
    if(inputData[i] != inputOld[i])
      return true;
  }
  return false;
}

/* 
 * Function: printData
 * Accepts a byte and the name of the register. Prints
 * the name of the register submitted and prints the byte
 * in the expected format returned from readPISO() or sent
 * to sendSIPO()
 */
void printData(byte data, String regName, int regNum){
  Serial.print("    ");                        
  Serial.print(regName);            // Print the register name
  Serial.print(regNum);             // Print the register number        
  Serial.print(": ");
  for (int j=0; j<8; j++){          // For each bit...
    Serial.print(bitRead(data, j)); // Print the data to/from the
    Serial.print(" ");              // shift register in LSBFIRST order
  }
  Serial.println();
}

/* 
 * Function: pulsePin
 * Accepts a digital output pin and a pulse time
 * Pulses a pin LOW for the specified pulse time
 * in microseconds.
 */
void pulsePin(int pinName, int pulseTime){
  digitalWrite(pinName, LOW);
  delayMicroseconds(pulseTime);
  digitalWrite(pinName, HIGH);
  delayMicroseconds(pulseTime);
}

/* 
 * Function: readPISO
 * Accepts a byte array and writes the data from the PISO 
 * registers to the byte array.
 * Per the datasheet, data is loaded MSBFIRST, so the first 
 * bit in will be received from pin H in the PISO register chain.
 * 
 * If you had two PISO registers daisy chained, with 0A denoting
 * Pin A on the first PISO register and 1A denoting Pin A on the
 * second PISO register, the PHYSICAL order of pins being read
 * from would be the following:
 * 
 * Bit#: 00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15
 * Pin#: 0H 0G 0F 0E 0D 0C 0B 0A 1H 1G 1F 1E 1D 1C 1B 1A
 * 
 * However, this function writes the data to the byte array
 * in LSBFIRST order for ease of programming and readability.
 * 
 * Example:
 * Suppose you have two PISO registers in a chain and the states
 * of the PISO registers are the following...
 * 
 * Pin States: A B C D E F G H
 *     PISO#0: 0 1 1 1 1 1 1 1
 *     PISO#1: 0 0 0 0 0 0 0 1
 * 
 * The resulting input written to the byte array will be...
 * byte data = {127,1}; // (b01111111,b00000001)
 */
void readPISO(byte data[numPISO]){
  digitalWrite(clockPin, HIGH);                     // Data is shifted when clock pin changes from LOW to HIGH, so ensure it starts HIGH
  pulsePin(loadPin, 5);                             // Pulse load pin to snapshot all PISO pin states and start at the beginning
  for(int i=0; i<numPISO; i++){                     // For each PISO register...
    for(int j=7; j>=0; j--){                        // For each bit in the PISO register...
      bitWrite(data[i], j, digitalRead(dataInPin)); // Write the current bit to the corresponding bit in the input variable
      pulsePin(clockPin, 5);                        // Pulse the clock to shift the next bit in from the PISO register
    }
  }
}

/* 
 * Function: sendSIPO
 * Accepts a byte array in LSBFIRST order and writes the data to 
 * the SIPO registers.
 * 
 * Per the datasheet, data is written MSBFIRST, so the first bit
 * written will be written to pin H of the last SIPO register in 
 * the chain.
 * 
 * If you have two SIPO registers daisy chained, with 0A denoting
 * Pin A on the first SIPO register and 1A denoting Pin A on the
 * second SIPO register, the PHYSICAL order of pins being written
 * to will be the following:
 * 
 * Bit#: 00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15
 * Pin#: 1H 1G 1F 1E 1D 1C 1B 1A 0H 0G 0F 0E 0D 0C 0B 0A
 * 
 * However, this function reads the data from the byte array
 * in MSBFIRST order for ease of programming and readability.
 * 
 * Example:
 * Suppose you have two SIPO registers in a chain and you input the
 * following bytes...
 * 
 * byte data = {127,1}; // (b01111111,b00000001)
 * 
 * The resulting output on the SIPO registers will be...
 * Pin States: A B C D E F G H
 *     SIPO#0: 0 1 1 1 1 1 1 1
 *     SIPO#1: 0 0 0 0 0 0 0 1
 */
void sendSIPO(byte data[numSIPO]){
  for(int i=numSIPO-1; i>=0; i--){                    // For each SIPO register...
    if(DEBUG){
    }
    for(int j=7; j>=0; j--){                          // For each bit in the register...
       digitalWrite(dataOutPin, bitRead(data[i], j)); // Read the current bit of the input variable and write it to the SIPO data pin
       pulsePin(clockPin, 5);                         // Pulse the clock to shift the bit out to SIPO registers
    }
  }
  pulsePin(latchPin, 5);                              // Pulse the latch to allow new data to appear on SIPO registers
}
