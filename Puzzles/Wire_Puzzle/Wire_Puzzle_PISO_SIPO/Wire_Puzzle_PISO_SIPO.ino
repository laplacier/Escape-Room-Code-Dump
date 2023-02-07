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
 * This example is designed for use with SN74HC165N  and SN74HC595N
 * shift registers.
 * 
 * PISO - Parallel in, Serial out. The SN74HC165N accepts 8 inputs
 * in parallel and outputs their states on one serial line
 * SIPO - Serial in, parallel out. The SN74HC595N accepts 1 serial
 * input byte and outputs each bit to 8 outputs in parallel
 * 
 * This implementation requires 1 IO pin assigment for each
 * wire connection and 5 IO pins to control any amount of PISO
 * and SIPO registers. The last PISO registercin the chain will 
 * have nothing connected to pin 10, and the last SIPOregister 
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
 * 
 * by Adam Billingsley
 * created 6 Feb, 2023
 */

#define DEBUG true       // Prints debug information to serial if true
#define numWires   8     // Total wires in the puzzle solution
#define numPISO    1     // Number of PISO registers in daisy chain
#define numSIPO    1     // Number of SIPO registers in daisy chain
#define clockPin   7     // Pin 2 on all SN74HC165N and Pin 11 on all SN74HC595N
#define dataInPin  3     // Pin 7 on FIRST SN74HC165
#define loadPin    2     // Pin 1 on all SN74HC165
#define latchPin   4     // Pin 12 on all SN74HC595N
#define dataOutPin 6     // Pin 14 on FIRST SN74HC595N
byte inputData[numPISO]; // Stores PISO data

/*
 * Pin assignments per correct connection.
 * Left hand value is the microcontroller IO pin
 * Right hand value is the PISO register bit
 * 
 * PISO data is stored LSBFIRST, so from bits 0-7
 * the order is ABCDEFGH, A=0, B=1, etc.
 * SIPO data is stored LSBFIRST, so from bits 0-7
 * the order is ABCDEFGH, A=0, B=1, etc.
 */
const int solution[numWires][2] = {
  {0,0},
  {1,1},
  {2,2},
  {3,3},
  {4,4},
  {5,5},
  {6,6},
  {7,7},
};

/* 
 * Function: checkWires
 * Checks all wires, allows optional code to trigger per wire
 * Returns true if all wires are connected correctly
 */
bool checkWires(){
  bool flag_solved = true;                  // Flag to return puzzle solved state. True until proven false
  byte outputData[numSIPO];                 // Byte array stores data to send to SIPO registers
  for(int i=0; i<numSIPO; i++){             // For each byte of SIPO data...
    outputData[i] = 255;                    // Initialize all bits to HIGH
  }
  for(int i=0; i<numWires; i++){            // For each wire...

    //Pull first end of wire LOW
    const int columnSIPO = solution[i][0] / 8;    // Determine which SIPO byte to write
    const byte rowSIPO = solution[i][0] % 8;      // Determine which bit of the SIPO byte to write
    bitWrite(outputData[columnSIPO], rowSIPO, 0); // Write 0 to the selected output data bit
    sendSIPO(outputData);                         // Write new states to SIPO registers

    //Read state of second end of wire
    readPISO(inputData);                         // Grab the states of the PISO registers
    const int columnPISO = solution[i][1] / 8;   // Determine which PISO byte to read
    const byte rowPISO = solution[i][1] % 8;     // Determine which bit of the PISO byte to read
    if(bitRead(inputData[columnPISO], rowPISO)){ // If the second end of the wire detects the LOW pull...
                                                 // Optional: do anything to reward the single correct connection!
      if(DEBUG){
        Serial.print("Correct connection detected between SIPO bit ");
        Serial.print(solution[i][0]);
        Serial.print(" and PISO bit ");
        Serial.println(solution[i][1]);
      }
    }
    else{
      flag_solved = false;                        // The wire is connected improperly, don't solve the game
    }

    //Pull first end of wire HIGH again
    bitWrite(outputData[columnSIPO], rowSIPO, 1); // Write 1 to the selected output data bit
    sendSIPO(outputData);                         // Write new states to SIPO registers
  }
  return flag_solved;                             // Return the solved state of the puzzle
}

void setup(){
  if(DEBUG)
    Serial.begin(19200);

  // Set all the pins of SN74HC165N
  pinMode (loadPin, OUTPUT);
  pinMode (dataInPin, INPUT);

  // Set all the pins of SN74HC595N
  pinMode(latchPin, OUTPUT);
  pinMode(dataOutPin, OUTPUT);

  // The clock pin is shared between ALL registers
  pinMode (clockPin, OUTPUT);
}

void loop(){
  if(checkWires()){ //If the wires are all connected properly...
    //The puzzle is solved!
    if(DEBUG)
      Serial.println("All wires connected, puzzle solved!");
  }
  delay(500);
}
