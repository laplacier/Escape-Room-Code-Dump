/*
 * ESCAPE ROOM WIRE PUZZLE DEMO
 * 
 * You have a large array of plugs. Players have wires they must
 * connect to the plugs, but only a certain combination of 
 * connections between plugs will trigger the solution and allow
 * the players to progress through the game!
 * 
 * This implementation allows for seamless detection of wires
 * connected correctly with minimal external components.
 * Tired of building labor intensive resistor or keypad arrays,
 * having them degrade over time, and reading noisy analog values?
 * Me too!
 * 
 * This example is designed for use with SN74HC165N shift registers.
 * 
 * PISO - Parallel in, Serial out. The SN74HC165N accepts 8 inputs
 * in parallel and outputs their states on one serial line
 * 
 * This implementation requires 1 IO pin assigment for each
 * wire connection and 3 IO pins to control any amount of PISO
 * registers daisy chained together. The last PISO register
 * in the chain will have nothing connected to pin 10.
 * Each input pin of the shift register must be pulled high
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
 * 
 * by Adam Billingsley
 * created 6 Feb, 2023
 */

#define DEBUG true       // Prints debug information to serial if true
#define numWires  8      // Total wires in the puzzle solution
#define numPISO   1      // Number of PISO registers in daisy chain
#define clockPin  8      // Pin 2 of SN74HC165
#define dataInPin 9      // Pin 7 of SN74HC165
#define loadPin   10     // Pin 1 of SN74HC165
byte inputData[numPISO]; // Stores PISO data

/*
 * Pin assignments per correct connection.
 * Left hand value is the microcontroller IO pin
 * Right hand value is the PISO register bit
 * 
 * PISO data is stored LSBFIRST, so from bits 0-7
 * the order is ABCDEFGH, A=0, B=1, etc.
 */
const int solution[numWires][2] = {
  {1,0},
  {2,1},
  {3,2},
  {4,3},
  {5,4},
  {6,5},
  {7,6},
  {8,7},
};

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
 * Function: checkWires
 * Checks all wires, allows optional code to trigger per wire
 * Returns true if all wires are connected correctly
 */
bool checkWires(){
  bool flag_solved = true;                  // Flag to return puzzle solved state. True until proven false
  for(int i=0; i<numWires; i++){            // For each wire...
    pinMode(solution[i][0], OUTPUT);        // Reassign first end of wire as an output
    digitalWrite(solution[i][0], LOW);      // Pull first end of wire LOW
    readPISO(inputData);                    // Grab the states of the PISO registers
    const int column = solution[i][1] / 8;  // Determine which byte to read
    const byte row = solution[i][1] % 8;    // Determine which bit to read
    if(bitRead(inputData[column], row)){    // If the second end of the wire detects the LOW pull...
                                            // Optional: do anything to reward the single correct connection!
      if(DEBUG){
        Serial.print("Correct connection detected between pin ");
        Serial.print(solution[i][0]);
        Serial.print(" and PISO bit ");
        Serial.println(solution[i][1]);
      }
    }
    else{
      flag_solved = false;                 // The wire is connected improperly, don't solve the game
    }
    pinMode(solution[i][0], INPUT_PULLUP); // Pull the first end of the wire HIGH and check the next wire
  }
  return flag_solved;                      // Return the solved state of the puzzle
}

void setup(){
  if(DEBUG)
    Serial.begin(19200);
  
  //All wire plugs initialized as inputs
  for(int i=0; i<numWires; i++){
    pinMode(solution[i][0], INPUT_PULLUP);
  }

  //PISO register pins
  pinMode(loadPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataInPin, INPUT);
}

void loop(){
  if(checkWires()){ //If the wires are all connected properly...
    //The puzzle is solved!
    if(DEBUG)
      Serial.println("All wires connected, puzzle solved!");
  }
  delay(500);
}
