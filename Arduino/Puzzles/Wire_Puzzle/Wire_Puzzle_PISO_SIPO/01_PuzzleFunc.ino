/* 
 * Function: checkWires
 * Checks all wires, allows optional code to trigger per wire
 * Returns true if all wires are connected correctly
 */
bool checkWires(){
  bool flag_solved = true;                        // Flag that solves the puzzle. True until proven false
  byte outputData[numSIPO];                       // Byte array stores data to send to SIPO registers
  for(int i=0; i<numSIPO; i++){                   // For each byte of SIPO data...
    outputData[i] = 255;                          // Initialize all bits to HIGH
  }
  for(int i=0; i<numWires; i++){                  // For each wire...
    //Pull first end of wire LOW
    const int columnSIPO = solution[i][0] / 8;    // Determine which SIPO byte to write
    const byte rowSIPO = solution[i][0] % 8;      // Determine which bit of the SIPO byte to write
    bitWrite(outputData[columnSIPO], rowSIPO, 0); // Write 0 to the SIPO bit
    sendSIPO(outputData);                         // Write new state to SIPO registers

    //Read state of second end of wire
    readPISO(inputData);                          // Grab the states of the PISO registers
    const int columnPISO = solution[i][1] / 8;    // Determine which PISO byte to read
    const byte rowPISO = solution[i][1] % 8;      // Determine which bit of the PISO byte to read
    if(bitRead(inputData[columnPISO], rowPISO)){  // If the second end of the wire detects the LOW pull...
                                                  // Optional: do anything to reward the single correct connection!
      if(DEBUG){
        Serial.print("Correct connection detected between SIPO bit ");
        Serial.print(solution[i][0]);
        Serial.print(" and PISO bit ");
        Serial.println(solution[i][1]);
      }
    }
    else{
      flag_solved = false;                        // A wire is connected improperly, don't solve the game
    }

    //Pull first end of wire HIGH again
    bitWrite(outputData[columnSIPO], rowSIPO, 1); // Write 1 to the selected output data bit
    sendSIPO(outputData);                         // Write new states to SIPO registers
  }
  return flag_solved;                             // Return the solved state of the puzzle
}
