/*
 * 7-SEGMENT DISPLAY WITH SN74HC595N DEMO
 * 
 * SIPO - Serial in, parallel out. The SN74HC595N accepts 1 serial
 * input byte and outputs each bit to 8 outputs in parallel
 * 
 * You have a lot of digital output connections to make, but
 * not enough pins on your microcontroller to connect them all.
 * Rather than buying a more expensive microcontroller with more
 * pins, try adding some inexpensive SIPO registers!
 * 
 * Use SIPO registers for:
 * -Expanding the number of output pins available
 * -Outputs that need to be controlled HIGH and LOW
 * 
 * Do NOT use SIPO registers for:
 * -Output pins with extremely tight timing requirements (us)
 * -Analog data
 * -Communication protocols through a SIPO pin (I2C, SPI, etc)
 * 
 * You will need 3 IO pins to control the SIPO register
 * connected to the 7 segment display.
 * 
 *      SN74HC595N          7-Segment Display
 *        __ __                   VCC
 *       |  U  |               G F | A B
 *     B-|1  16|-VCC           |_|_|_|_|
 *     C-|2  15|-A            |   ====  |
 *     D-|3  14|-DATA         |  ||  || |
 *     E-|4  13|-GND          |   ====  |
 *     F-|5  12|-LATCH        |  ||  || |
 *     G-|6  11|-CLK          |   ==== o|
 *     H-|7  10|-VCC          `---------'
 *   GND-|8   9|-              | | | | |
 *       |_____|               E D | C H
 *                                VCC
 *      
 * by Adam Billingsley
 * created 8 Feb, 2023
 */

#define numSIPO    1 // Number of SIPO registers in daisy chain
#define latchPin   4 // Pin 12 on all SN74HC595N
#define clockPin   7 // Pin 11 on all SN74HC595N
#define dataOutPin 6 // Pin 14 on FIRST SN74HC595N

/* 
 * Function: printSIPO
 * Accepts a byte array and prints the expected output of
 * the SIPO registers when inputted to sendSIPO()
 */
void printSIPO(byte data[numSIPO]){
  Serial.println("Pin States: A B C D E F G H"); // Print header row in LSBFIRST order
  for(int i=0; i<numSIPO; i++){                  // For each byte..
    Serial.print("    SIPO#");                   // Print the expected SIPO register number
    Serial.print(i);
    Serial.print(": ");
    for (int j=0; j<8; j++){                     // For each bit...
      Serial.print(bitRead(data[i], j));         // Print the expected state of the SIPO
      Serial.print(" ");                         // register pin in LSBFIRST order
    }
    Serial.println();
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
 * Function: sendSIPO
 * Accepts a byte array in LSBFIRST order and writes the data to 
 * the SIPO registers.
 * 
 * Per the datasheet, data is written MSBFIRST, so the first bit
 * written will be written to pin H of the last SIPO register in 
 * the chain.
 * 
 * If you had two SIPO registers daisy chained, with 0A denoting
 * Pin A on the first SIPO register and 1A denoting Pin A on the
 * second SIPO register, the PHYSICAL order of pins being written
 * to would be the following:
 * 
 * Bit#: 00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15
 * Pin#: 1H 1G 1F 1E 1D 1C 1B 1A 0H 0G 0F 0E 0D 0C 0B 0A
 * 
 * However, this function reads the data from the byte array
 * in MSBFIRST order for ease of programming and readability.
 * 
 * Example:
 * Suppose you have two SIPO registers in a chain and you input the
 * following...
 * 
 * byte data = {127,1}; // (b01111111,b00000001)
 * 
 * The resulting output on the SIPO registers will be...
 * Pin States: A B C D E F G H
 *     SIPO#0: 0 1 1 1 1 1 1 1
 *     SIPO#1: 0 0 0 0 0 0 0 1
 */
void sendSIPO(byte data[numSIPO]){
  printSIPO(data);                                    // Print expected output to serial
  for(int i=numSIPO-1; i>=0; i--){                    // For each SIPO register...
    for(int j=7; j>=0; j--){                          // For each bit in the register...
       digitalWrite(dataOutPin, bitRead(data[i], j)); // Read the current bit of the input variable and write it to the SIPO data pin
       pulsePin(clockPin, 5);                         // Pulse the clock to shift the bit out to SIPO registers
    }
  }
  pulsePin(latchPin, 5);                              // Pulse the latch to allow new data to appear on SIPO registers
}


/* 
 * Function: cyclePins
 * Cycles through all SIPO pins, bringing each pin
 * HIGH then back to LOW starting from the first pin
 * (SIPO#0, pin A) in the chain of registers.
 */
void cyclePins(){
  byte outputData[numSIPO];
  for(int i=0; i<numSIPO; i++){    // For each SIPO register...
    for (int i=0; i<8; i++){       // For each pin of the SIPO register...
      bitSet(outputData[0], i);    // Set the selected pin HIGH
      sendSIPO(outputData);        // Output the data on the SIPO registers
      delay(1000);
      bitClear(outputData[0], i);  // Set the selected pin LOW
      sendSIPO(outputData);        // Output the data on the SIPO registers
      delay(400);
    }
  }
}

void setup() 
{
  Serial.begin(19200);
  Serial.println();
  Serial.println("-----74HC595 demo-----");
  // Set all the pins of SN74HC595N as OUTPUT
  pinMode(latchPin, OUTPUT);
  pinMode(dataOutPin, OUTPUT);  
  pinMode(clockPin, OUTPUT);
}

void loop() 
{
  cyclePins();
}
