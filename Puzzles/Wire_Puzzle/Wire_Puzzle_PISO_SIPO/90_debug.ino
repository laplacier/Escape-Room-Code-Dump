/* 
 * Function: isWireNew
 * Checks all correct wires, returns position of
 * change in index=1 when a change is detected.
 */
byte isWireNew(){
  for(byte i=0; i<numWires; i++){
    if(wiresOld[i] != wiresCorrect[i])
      return i;
  }
  return 0;
}

/* 
 * Function: debug
 * Prints all shift register data when any change
 * is detected.
 * Prints relevant information about game states
 * when detected.
 */
void debug(){
  if(isDataNew()){                           // If any shift register bits changed...
    const String namePISO = "PISO#";         // Create register name prefixes to print
    const String nameSIPO = "SIPO#";
    for(int i=0; i<numPISO; i++){            // For each PISO register...
      printData(inputData[i], namePISO, i);  // Print the selected register byte
      inputOld[i] = inputData[i];            // Store byte as previous byte
    }
    for(int i=numSIPO-1; i>=0; i--){         // For each SIPO register...
      printData(outputData[i], nameSIPO, i); // Print expected output to serial
      outputOld[i] = outputData[i];          // Store byte as previous byte
    }
  }

  const byte connectedWire = isWireNew();
  if(connectedWire){                                           // If a correct wire connection changed...
    if(wiresCorrect[connectedWire-1]){                         // If the connection is correct...
      Serial.print("correct connection detected between ");    // Print that it was correct
    }
    Serial.print("wires ");
    Serial.print(solution[connectedWire-1][0]);
    Serial.print(" and ");
    Serial.print(solution[connectedWire-1][1]);
    if(!wiresCorrect[connectedWire-1]){                        // If the connection is incorrect...
      Serial.print(" disconnected.");                          // Print that it was incorrect
    }
    Serial.println(".");
    if(flag_solved)                                            // If all connections correct...
      Serial.println("All wires connected, puzzle solved!");   // Print that it was solved
    wiresOld[connectedWire-1] = wiresCorrect[connectedWire-1]; // Store wire states as previous states
  }
}
