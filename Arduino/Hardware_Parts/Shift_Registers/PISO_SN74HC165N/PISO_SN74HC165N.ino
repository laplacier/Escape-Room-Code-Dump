/*
 * SN74HC165N DEMO
 * 
 * PISO - Parallel in, Serial out. The SN74HC165N accepts 8 inputs
 * in parallel and outputs their states on one serial line
 * 
 * You have a lot of digital input connections to make, but
 * not enough pins on your microcontroller to connect them all.
 * Rather than buying a more expensive microcontroller with more
 * pins, try adding some inexpensive PISO registers!
 * 
 * Use PISO registers for:
 * -Expanding the number of input pins available
 * -Inputs that need to read a HIGH and LOW state
 * 
 * Do NOT use PISO registers for:
 * -Input pins with extremely tight timing requirements (us)
 * -Analog data
 * -Communication protocols through a PISO pin (I2C, SPI, etc)
 * 
 * You will need 3 IO pins to control any amount of PISO
 * registers daisy chained together. The last PISO register
 * in the chain will have nothing connected to pin 10.
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
 *      
 * by Adam Billingsley
 * created 7 Feb, 2023
 */

#define numPISO     1    // Number of PISO registers in daisy chain
#define loadPin     2    // Pin 1 on all SN74HC165N
#define clockPin    7    // Pin 2 on all SN74HC165N
#define dataInPin   3    // Pin 9 on FIRST SN74HC165N
byte inputData[numPISO]; // Stores PISO data

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
 * Function: printPISO
 * Accepts a byte array and prints the input of
 * the PISO registers in the expected format returned 
 * from readPISO()
 */
void printPISO(byte data[numPISO]){
  Serial.println("Pin States: A B C D E F G H"); // Print header row in LSBFIRST order
  for(int i=0; i<numPISO; i++){                  // For each byte..
    Serial.print("    PISO#");                   // Print the expected PISO register number
    Serial.print(i);
    Serial.print(": ");
    for (int j=0; j<8; j++){                     // For each bit...
      Serial.print(bitRead(data[i], j));         // Print the state of the PISO
      Serial.print(" ");                         // register pin in LSBFIRST order
    }
    Serial.println();
  }
  Serial.println();
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

void setup() {
  Serial.begin(19200);
  Serial.println();
  Serial.print("-----74HC165 demo-----");
  // Set all the pins of SN74HC165N, only the data pin is INPUT
  pinMode(clockPin, OUTPUT);
  pinMode(loadPin, OUTPUT);
  pinMode(dataInPin, INPUT);
}

void loop() {
  readPISO(inputData);
  printPISO(inputData);
  delay(500);
}
