static const char BIT_LOOKUP[16] = { // Lookup table for number of bits set in a byte
  0, 1, 1, 2, 1, 2, 2, 3, 
  1, 2, 2, 3, 2, 3, 3, 4
};

/* 
 * Function: getLightCount
 * Takes a light pattern and returns number of lights that are on.
 * Uses bit shifting with a static lookup table to speed this process up
 * in case of large amounts of lights.
 */
byte getLightCount(int lights){
  byte count = 0;
  while(!lights){ // While ON lights still exist...
    count += BIT_LOOKUP[lights & 0x0F]; // Grab number of ON lights in current 4 lights
    lights >>= 4; // Change focus to the next 4 lights
  }
  return count; 
}
 
/* 
 * Function: generateSolution
 * Takes the globally defined number of switches and number of
 * desired correct switches. Generates an array of correct switches
 * and an array of their light patterns.
 */
void generateSolution(){
  int lightCombos = (2^numLight) - 1; // Total number of possible light combinations
  bool solutionSwitches[numSolution]; // Array of correct switches to turn on
  int solutionLights[numSolution]; // Array of light patterns for correct switches
  bool countSwitches[numSwitch]; // Count array to track switch occurences in solution
  bool countLights[lightCombos]; // Count array to track light pattern occurences in solution
  for (int i=0; i<numSwitch; i++){ // For each possible switch position...
    countSwitches[i] = 0; // Initialize occurences to 0
  }
  for(int i=0; i<lightCombos; i++){ // For each possible light pattern...
    countLights[i] = 0; // Initialize occurences to 0
  }
  for (int i=0; i<numSolution; i++){ // For each switch and light pattern solution...
    do {
      solutionSwitches[i] = random(0,numSwitch); // Generate a random index of a switch
    }while(countSwitches[solutionSwitches[i]]); // If the switch already exists in solution...
    solutionLights[i] = random(1,lightCombos-1); // Generate a light pattern, but not all OFF or ON
    do {
      solutionLights[i] = random(1,lightCombos-1); // Generate another light pattern, but not all OFF or ON
      if(!(getLightCount(solutionLights[i]) & 0x01)){ // If there are an even number of lights turned ON...
        solutionLights[i]++; // Make that number odd
      }
    }while(countLights[solutionLights[i]]); // If the light pattern already exists in solution...
  }
}

/*
EASY - All lights off when all switches off
-Seed RNG
-Store number of switches, numSwitch
-Store number of correct switches, numSolution

GENERATE LIGHT PATTERN OF CORRECT SWITCHES
-Generate numSolution numbers from 1 to (2^numSolution)-2 (NOT 0 to (2^numSolution)-1, we dont want all on or all off)
-For each number...
 -Count number of true bits
  -If number of true bits even...
    -Flip bit number[loop index]
-Perform nxn matrix transpose, you now have light combo for each switch.

HARD - Some lights on when all switches off
-Seed RNG
-Store number of switches, numSolution
-Generate a single number from 1 to (2^numSolution)-2 (NOT 0 to (2^numSolution)-1, we dont want all on or all off) to seed the initial state of the lights.
-Generate numSolution numbers from 1 to (2^numSolution)-2 (NOT 0 to (2^numSolution)-1, we dont want all on or all off)
-For each number...
  -Get initial light state
  -Count number of true bits
  -If number of true bits even and light state false...
   OR If number of true bits odd and light state true...
   (XOR first bit of counter with light state, invert, set as condition)
    -Flip bit number[loop index]
-Perform nxn matrix transpose, you now have light combo for each correct switch.

GENERATE INCORRECT DUMMY SWITCHES
------TODO

Transposing an nxn boolean matrix

[0,0], [0,1], ..., [0,n]
[1,0]
.
.
.
[n,0], [n,1], ..., [n,n]

left to right...
[0,0] <> [n,n]
[0,1] <> [n-1,n]
[0,2] <> [n-2,n]
.
.
.
[0,n] <> [n-n,n] ([0,n])

top to bottom...
[0,0] <> [n,n]
[1,0] <> [n,n-1]
[2,0] <> [n,n-2]
.
.
.
[n,0] <> [n,n-n] ([n,0])

01 02 03 04   16 15 14 13
05 06 07 08   12 11 10 09
09 10 11 12   08 07 06 05
13 14 15 16   04 03 02 01

Transpose each number, left to right, top to bottom, until (n^2)/2, rounded down, numbers are transposed.

SHORTCUTS:
boolean transpose: XOR both bools, if true, transpose.
divide by 2 rounded down: shift least significant bit of number out
 */
