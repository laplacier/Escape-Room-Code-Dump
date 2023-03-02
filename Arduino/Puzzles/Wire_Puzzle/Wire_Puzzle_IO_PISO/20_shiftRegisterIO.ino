/*
 * This example is designed for use with SN74HC165N shift registers.
 * 
 * PISO - Parallel in, Serial out. The SN74HC165N is a PISO
 * register. It accepts 8 inputs in parallel and outputs their 
 * states on one shiftable pin.
 * 
 * The last PISO register in the chain will have nothing 
 * connected to pin 10.
 * Each input pin of the PISO registers must be pulled high
 * with a resistor (recommended values between 1k-10k)
 * 
 * PISO#0 __ __
 *       |  U  |
 *  LOAD-|1  16|-VCC
 *   CLK-|2  15|-GND
 *     E-|3  14|-D
 *     F-|4  13|-C
 *     G-|5  12|-B
 *     H-|6  11|-A
 *      -|7  10|- to PISO#1 pin 9
 *   GND-|8   9|-DATA
 *       |_____|
 *      
 * PISO#1 __ __
 *       |  U  |
 *  LOAD-|1  16|-VCC
 *   CLK-|2  15|-GND
 *    E'-|3  14|-D'
 *    F'-|4  13|-C'
 *    G'-|5  12|-B'
 *    H'-|6  11|-A'
 *      -|7  10|- to PISO#2 pin 9, etc...
 *   GND-|8   9|- to PISO#0 pin 10
 *       |_____|
 */

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
