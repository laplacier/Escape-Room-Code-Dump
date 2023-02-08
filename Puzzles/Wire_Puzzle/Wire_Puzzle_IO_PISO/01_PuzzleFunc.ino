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
