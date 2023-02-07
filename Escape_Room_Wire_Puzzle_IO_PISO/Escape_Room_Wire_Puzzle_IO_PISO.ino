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
 * This example is designed for use with an SN74HC165N shift register.
 *       
 * SN74HC165N Connections
 *       __ __
 *      |  U  |
 * LOAD-|1  16|-VCC
 *  CLK-|2  15|-VCC(CE)
 *    E-|3  14|-D
 *    F-|4  13|-C
 *    G-|5  12|-B
 *    H-|6  11|-A
 * DATA-|7  10|-NC(SI)
 *  GND-|8   9|-NC(~DATA)
 *      |_____|
 * 
 * This implementation requires 1 IO pin assigment for each
 * wire connection and 3 IO pins for a PISO shift register.
 * Each input pin of the shift register must be pulled high
 * with a resistor (recommended values between 1k-10k)
 * 
 * by Adam Billingsley
 * created 6 Feb, 2023
 */

#define DEBUG true // Prints debug information to serial if true
#define numWires 8 // Total wires in the puzzle solution

#define clockPin 9 // Pin 2 of SN74HC165
#define dataPin 10 // Pin 7 of SN74HC165 (Pin 9 for inverse)
#define loadPin 11 // Pin 1 of SN74HC165

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
 * Function: readPISO()
 * Reads the states of each PISO input pin and returns
 * a byte value representing the states in LSBFIRST order.
 */
byte readPISO(){
  byte data;

  digitalWrite(loadPin, LOW);                // Pulse the load pin to load all pin states to serial
  delayMicroseconds(5);
  digitalWrite(loadPin, HIGH);
  delayMicroseconds(5);

  for(int i=0; i<8; i++){                    // For each bit of the PISO register...
    bitWrite(data, i, digitalRead(dataPin)); // Retrieve the current bit and store value in data
    digitalWrite(clockPin, HIGH);            // Pulse the clock to advance to the next bit
    delayMicroseconds(5);
    digitalWrite(clockPin, LOW);
    delayMicroseconds(5);
  }

  return data;                               // Return the data as a byte
}

/* 
 * Function: checkWires
 * Checks all wires, allows optional code to trigger per wire
 * Returns true if all wires are connected correctly
 */
bool checkWires(){
  bool flag_solved = true;                 // Flag to return puzzle solved state. True until proven false
  for(int i=0; i<numWires; i++){           // For each wire...
    pinMode(solution[i][0], OUTPUT);       // Reassign first end of wire as an output
    digitalWrite(solution[i][0], LOW);     // Pull first end of wire LOW
    byte data = readPISO();                // Grab the states of the PISO register bits
    if(bitRead(data, solution[i][1])){     // If the second end of the wire detects the LOW pull...
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
  
  //All wires initialized as inputs
  pinMode(1, INPUT_PULLUP);
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  pinMode(4, INPUT_PULLUP);
  pinMode(5, INPUT_PULLUP);
  pinMode(6, INPUT_PULLUP);
  pinMode(7, INPUT_PULLUP);
  pinMode(8, INPUT_PULLUP);

  //PISO register pins
  pinMode(loadPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, INPUT);
}

void loop(){
  if(checkWires()){ //If the wires are all connected properly...
    //The puzzle is solved!
    if(DEBUG)
      Serial.println("All wires connected, puzzle solved!");
  }
  delay(500);
}
