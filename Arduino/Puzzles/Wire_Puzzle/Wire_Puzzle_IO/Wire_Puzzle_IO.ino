/*
 * ESCAPE ROOM WIRE PUZZLE DEMO
 * 
 * You have a large array of plugs. Players have wires they must
 * connect to the plugs, but only a certain combination of 
 * connections between plugs will trigger the solution and allow
 * the players to progress through the game!
 * 
 * This implementation allows for seamless detection of wires
 * connected correctly without additional external components
 * (assuming your microcontroller supports INPUT_PULLUP).
 * Tired of building labor intensive resistor or keypad arrays,
 * having them degrade over time, and reading noisy analog values?
 * Me too!
 * 
 * This implementation requires 2 IO pin assigments for each
 * wire connection.
 * 
 * by Adam Billingsley
 * created  6 Feb, 2023
 * modified 7 Feb, 2023
 */

//-------------- SETTINGS & GLOBAL CONSTANTS -----------------//
#define DEBUG true // Prints debug information to serial if true
#define numWires 4 // Total wires in the puzzle solution


//------------------ PUZZLE SOLUTION -------------------------//
const int solution[numWires][2] = {
  {1,2}, // DIO pin 1, DIO pin 2
  {3,4}, // DIO pin 3, DIO pin 4
  {5,6}, // DIO pin 5, DIO pin 6
  {7,8}  // DIO pin 7, DIO pin 8
};

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
    if(!digitalRead(solution[i][1])){      // If the second end of the wire detects the LOW pull...
                                           // Optional: do anything to reward the single correct connection!
      if(DEBUG){
        Serial.print("Correct connection detected between pins ");
        Serial.print(solution[i][0]);
        Serial.print(" and ");
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

void setup() {
  if(DEBUG)
    Serial.begin(19200);
  
  //All wire plugs initialized as inputs
  for(int i=0; i<numWires; i++){
    pinMode(solution[i][0], INPUT_PULLUP);
    pinMode(solution[i][1], INPUT_PULLUP);
  }
}

void loop() {
  if(checkWires()){
    //The puzzle is solved!
    if(DEBUG)
      Serial.println("All wires connected, puzzle solved!");
  }
  delay(500);
}
