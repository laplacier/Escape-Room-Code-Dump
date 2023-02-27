static const char BIT_LOOKUP[16] = { // Lookup table for number of bits set in a byte
  0, 1, 1, 2, 1, 2, 2, 3, 
  1, 2, 2, 3, 2, 3, 3, 4
};

byte elimArr[uniqueNum];      // Stores the pattern data for an elimination sort
byte elimLower;               // Lower bound for elimination sort
byte elimUpper;               // Upper bound for elimination sort
byte numToGo;                 // Number of numbers left to generate for pattern
byte solution[patternLength]; // Stores valid pattern

/* 
 * Function: getLightCount
 * Takes a light pattern and returns number of lights that are on.
 * Uses bit shifting with a static lookup table to speed this process up
 * which helps tremendously with larger amounts of lights.
 */
byte getLightCount(int lights){
  byte count = 0;
  while(!lights){ // While ON lights still exist...
    count += BIT_LOOKUP[lights & 0x0F]; // Grab number of ON lights in current 4 lights
    lights >>= 4; // Remove those 4 lights and move to the next 4
  }
  return count; 
}

 /*
 * Function: shiftOutRight
 * This function accepts an elimination sort array, the position to finish 
 * shifting, and the lower bound of the shift. Shifts the array right from 
 * these two bounds. Does not resize the input array to increase speed; 
 * user must handle ignoring the unnecessary values.
 * 
 * Example: Lower bound = 0, position = 4
 * 
 *  - Start at i = (position - 1) = 3
 *  - Copy value at i to (i + 1)
 *  Index: 0  1  2  3  4  5  6
 *  Value: 1  3  5 [7]>9  0  8
 * 
 *  Index: 0  1  2  3  4  5  6
 *  Value: 1  3  5  7  7  0  8
 *  
 *  - Repeat until i < Lower bound, i = -1
 *  Index: 0  1  2  3  4  5  6
 *  Value: 1  1  3  5  7  0  8
 *  
 *  - Increment Lower bound by 1 and exit.
 */
void shiftOutRight(byte pos){
  for(int i=pos-1; i>=elimLower; i--){ // Starting at index=pos-1, for each index of the array to elimLower...
    elimArr[i+1] = elimArr[i];         // Shift index on the current index to the right.
  }
  elimLower++;                         // Increment lower bound by 1 to ignore useless data
}

/*
 * Function: shiftOutLeft
 * This function accepts an elimination sort array, the position to start 
 * shifting, and the upper bound of the shift. Shifts the array left from 
 * these two bounds. Does not resize the input array to increase speed; 
 * user must handle ignoring the unnecessary values.
 * 
 * Example: Upper bound = 6, position = 2
 * 
 *  - Start at i = position = 2
 *  - Copy value at (i + 1) to i
 *  Index: 0  1  2  3  4  5  6
 *  Value: 1  3 [5]<7  9  0  8
 * 
 *  Index: 0  1  2  3  4  5  6
 *  Value: 1  3  7  7  9  0  8
 *  
 *  - Repeat until i >= Upper bound, i = 6
 *  Index: 0  1  2  3  4  5  6
 *  Value: 1  3  7  9  0  8  8
 *  
 *  - Decrement Upper bound by 1 and exit.
 */
void shiftOutLeft(byte pos){
  for(int i=pos; i<elimUpper; i++){ // Starting at index=pos, for each index of the array to the upper bound...
    elimArr[i] = elimArr[i+1];      // Shift index on the right to the current index.
  }
  elimUpper--;                      // Decrement upper bound by 1 to ignore useless data
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
  int solutionLights[numSolution];    // Array of light patterns for correct switches
  byte elimSwitches[numSwitch];       // Elimination sort array to track remaining switch indexes
  bool countLights[lightCombos];      // Count array to track light pattern occurences in solution
  for (int i=0; i<numSwitch; i++){    // For each possible switch...
    elimSwitches[i] = i;              // Initialize elimination sort for selecting correct switches
  }
  for(int i=0; i<lightCombos; i++){   // For each possible light pattern...
    countLights[i] = 0;               // Initialize occurences to 0
  }
  for (int i=0; i<numSolution; i++){  // For each switch and light pattern solution...
    do {
      solutionSwitches[i] = random(0,numSwitch); // Generate a random index of a switch
    }while(countSwitches[solutionSwitches[i]]);  // If the switch already exists in solution...
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
