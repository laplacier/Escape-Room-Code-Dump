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
}
