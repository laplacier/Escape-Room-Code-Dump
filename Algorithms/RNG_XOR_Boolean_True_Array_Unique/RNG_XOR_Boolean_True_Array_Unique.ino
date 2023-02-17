/*
 * XOR BOOLEAN TRUE ARRAY UNIQUE
 * 
 * Inputs:
 *   - Number of arrays involved in the solution.
 *   - Total number of arrays to generate.
 *   - Array size
 * 
 * This algorithm will generate a random assortment of arrays with the 
 * following conditions:
 *   - The arrays will all be unique
 *   - No array will contain all 0s or all 1s
 *   - No array will contain a single 1.
 *   - There will be a single, unique solution for setting all bools to 1
 *   - No array will contain two unique bools that are 1. AKA, it cannot
 *     be the only correct switch that sets two certain bools to 1.
 * 
 * Examples:
 *   - Inputs:
 *     - Number of correct arrays: 3
 *     - Total arrays: 4
 *     - Array size: 5
 *   - Patterns:
 *     - 0 1 0 1 1 Valid, unique solution
 *       1 0 1 1 0 2nd, 3rd, 4th row
 *       0 1 1 1 0
 *       0 0 1 1 1
 *       
 *     - 1 1 0 0 1 Invalid, multiple solutions.
 *       1 1 1 0 1 1st, 2nd, 3rd row
 *       1 1 0 1 1 1st, 4th row - Not number of correct arrays
 *       0 0 1 1 0
 *       
 *     - 1 0 0 0 1 Invalid, multiple solutions, duplicate arrays
 *       1 0 0 0 1 1st, 3nd, 4rd row
 *       1 0 1 0 1 2nd, 3rd, 4th row
 *       1 1 0 1 1
 *       
 * This algorithm is useful for:
 *   - Generating a solution to a puzzle containing elements with outputs 
 *     that cancel each other out.
 *   - Generating a solution with no repeat inputs, to ensure appropriate
 *     complexity for a randomly generated puzzle.
 *   - Generating a unique solution to a puzzle with cause and effect
 *     elements.
 *     
 * by Adam Billingsley
 * created  17 Feb, 2023
 */

 /*==========================GLOBAL SETTINGS============================//
 * REQUIREMENTS:
 *   - numSolution must be less than or equal to numRow and greater than 0.
 *   - numRow must be 3 or greater.
 *   - arrSize cannot exceed 255. This is an artificial limit chosen
 *     for practical application. This algorithm can, if repreogrammed, 
 *     extend to infinite numbers and lengths.
 */
#define DEBUG false
#define numSolution 3  // Number of rows in the unique solution
#define numRow      5  // Total number of rows
#define arrSize     5  // Total number of boolean vals in a row
static const int combos = math.pow(2,arrSize); //Calculate the total number of cominbinations possible for an array
static const char BIT_LOOKUP[16] = { // Lookup table for number of bits set in a byte
  0, 1, 1, 2, 1, 2, 2, 3, 
  1, 2, 2, 3, 2, 3, 3, 4
};
byte solution[numRow]; // Stores valid pattern

void debug(byte state, int randNum, int pos){
  for(int i=0; i<numRow; i++){
    Serial.print(solution[i]);  
    Serial.print(" ");
  }
  Serial.print("Number to insert: ");
  Serial.print(randNum);
  Serial.print(" Starting at index: ");
  Serial.println(pos);
}

void debug(byte state, byte arr[numSolution], int lower, int upper){
  for(int i=0; i<numRow; i++){
    Serial.print(arr[i]);
    Serial.print(" ");
  }
  Serial.print("Lower Bound: ");
  Serial.print(lower);
  Serial.print(" Upper Bound: ");
  Serial.println(upper);
}

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
 * Function: shiftOutLeft
 * This function accepts a resevoir sampling array, the position to start 
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
void shiftOutLeft(int arr[combos], int *upper, int pos){
  for(int i=pos; i<*upper; i++){ // Starting at index=pos, for each index of the array to the upper bound...
    arr[i] = arr[i+1];      // Shift index on the right to the current index.
  }
  (*upper)--;                      // Decrement upper bound by 1 to ignore useless data
}

/*
 * Function: shiftOutRight
 * This function accepts a resevoir sampling array, the position to finish 
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
void shiftOutRight(int arr[combos], int *lower, int pos){
  for(int i=pos-1; i>=*lower; i--){ // Starting at index=pos-1, for each index of the array to resLower...
    arr[i+1] = arr[i];         // Shift index on the current index to the right.
  }
  (*lower)++;                         // Increment lower bound by 1 to ignore useless data
}

/*
 * Function: shiftInRight
 * This function accepts an array, the number to insert, the position 
 * to insert the number, and the upper bound of the shift. Shifts the 
 * array right from these two bounds. Does not resize the input array 
 * to increase speed; user must handle ignoring the unnecessary values.
 * 
 * Example: numRow - numToGo - 1 = 5, position = 3, insert = 3
 * 
 *  - Start at i = position = 3
 *  - Store value at i to temp var
 *  - Copy value to insert to i
 *  - Copy temp var to value to insert
 *  
 *  Insert: 3, Temp: 7
 *  Index: 0  1  2  3  4  5  6  7  8
 *  Value: 1  3  5 [7] 9  4  0  0  0
 * 
 *  Insert: 7, Temp: 7
 *  Index: 0  1  2  3  4  5  6  7  8
 *  Value: 1  3  5 [3] 9  4  0  0  0
 *  
 *  - Repeat until i > numRow - numToGo
 *  Index: 0  1  2  3  4  5  6  7  8
 *  Value: 1  3  5  3  7  9  4  0  0
 *  
 *  - Increment Upper bound by 1 and exit.
 */
void shiftInRight(int insert, int pos, int upper){
  int temp;
  for(int i=pos; i<=upper; i++){   // Starting at index=pos, for each index of the array that has a pattern value...
    temp = solution[i];            // Store current number in a temp var
    solution[i] = insert;          // Insert the shifted number into the pattern
    insert = temp;                 // Prepare the temp number to be inserted on next loop
  }
}

/*
 * Function: resSort
 * This function accepts a resevoir sampling array, a lower bound, and an
 * upper bound. Generates a random number between the lower and upper bound 
 * and performs a single resevoir sample. Returns a post processed resevoir 
 * sampling array and new lower and upper bounds.
 * 
 * Resevoir sampling is effective for speed when:
 *  - Attempting to find a unique number.
 *  - The number of unique values is large.
 * 
 * In a traditional loop to check each previous number, we can be checking up 
 * to n^2 numbers for each number, with no guarantee of a unique value. In 
 * resevoir sampling, no matter how many unique numbers there are, a generation 
 * of a unique number in resevoir sampling is a single array index check with 
 * a guaranteed unique value.
 * 
 * Time complexity of:
 *   - Traditional sort - O(n^2)
 *   - Resevoir sampling - O(n + n/2) = O(n)
 * Space complexity of:
 *   - Traditional sort - O(n)
 *   - Resevoir sampling - O(2n) = O(n)
 * 
 * However, resevoir sampling, by nature, does not create duplicate numbers. 
 * This method cannot be used for generating an amount of numbers greater than 
 * the number of unique numbers.
 * 
 * In our all numbers neighbors unique example, this method is used whenever 
 * it is applicable due to its speed.
 */
byte resSort(byte arr[numSolution], byte *upper, byte *lower){
  int pos = random(*lower, (*upper)+1);     // Randomly select an index of a number to pull for resevoir sampling
  int num = arr[pos];                       // Store that number
  if(arr[pos-(*lower)] < arr[(*upper)-pos]) // If that number is closer to the left side of the resevoir sampling array...
    shiftOutRight(arr, lower, pos);         // Shift resevoir sampling array right
  else 
    shiftOutLeft(arr, upper, pos);          // Otherwise, shift resevoir sampling array left
  return num;                               // Return the number shifted out of the resevoir sampling array
}

/*
 * Function: generatePattern 
 * This function generates a random all numbers, neighbors unique pattern.
 * It utilizes resevoir sampling to ensure all unique numbers are present
 * in the pattern. Afterwards, it generates random numbers from 0 to 
 * numSolution-1 and attempts to insert them into a random location in the
 * pattern, following the unique neighbor rule. It does this until the
 * desired pattern length has been reached.
 */

void generatePattern(){
  int resArr[combos];         // Stores the pattern data for resevoir sampling
  for(int i=0; i<combos; i++) // For each index in the resevoir sampling array...
    resArr[i] = i;             // Initialize the array with every unique number
  int resLower = 3;           // Lower bound for resevoir sampling
  int resUpper = combos-1;    // Upper bound for resevoir sampling
  int numToGo = numRow;       // Number of numbers left to generate for pattern

  /*********************************************************/
  /***************\PERFORM RESEVOIR SAMPLING/***************/
  /*********************************************************/
  for(int i=0; i<numSolution; i++){                     // For each unique number...
    if(DEBUG)
      debug(0, resArr, resUpper, resLower);
    solution[i] = resSort(resArr, &resUpper, &resLower); // Perform a resevoir sample and store the selected number
    numToGo--;                                           // Denote one less number left to generate
  }
  while(numToGo){                                        // If we still have numbers to generate...
    int randNum = random(3, combos);                    // Generate a random number from 3 to combos - 1
    int upperLimit = numRow - numToGo;                  // Stores index of the end of the generated array
    int pos = random(0, upperLimit);                    // Generate a random position within the bounds of generated numbers
    bool flag_inserted = false;                          // Flag checks if number has been inserted into pattern
    if(DEBUG)
      debug(1, randNum, pos);
    /********************************************************/
    /*******\ALL INSERTION NICHE CASES AND CONDITIONS/*******/
    /********************************************************/
    while(!flag_inserted){                                     // While the number has not been inserted into the pattern...
      if(pos == upperLimit - 1 && randNum != solution[pos]){   // If the new position is at the final generated number and is a valid insertion...
        shiftInRight(randNum, pos+1, pos+1);                   // Insert number at the end
        flag_inserted = true;
      }
      else if(pos > upperLimit - 1 && randNum != solution[0]){ // If the new position is outside the bounds of generated numbers and can be inserted at the beginning...
        shiftInRight(randNum, 0, upperLimit);                  // Insert number at beginning
        flag_inserted = true;
      }
      else if(pos > upperLimit && randNum == solution[0]){     // Otherwise, if the new position is outside the bounds of generated numbers and the first number is an identical neighbor...
        pos = 1;                                               // Move the position to +1 after the beginning  
      }
      /*******************************************************/
      /*******\STANDARD INSERTION CASES AND CONDITIONS/*******/
      /*******************************************************/
      else if(randNum == solution[pos] || randNum == solution[pos+1]){ // If number has an identical neighbor...
        pos += 2;                                                      // Increment the position to check the next pair of neighbors
      }
      else{                                                            // If number has unique neighbors...
        shiftInRight(randNum, pos+1, upperLimit);                      // Insert the number between them
        flag_inserted = true;
      }
    }
    numToGo--; // One less number to generate
  }
}

/*
 * Function: errorCheck 
 * This function halts the code from executing during setup if
 * a setting is configured incorrectly. Provides serial print
 * out of the issue.
 */
void errorCheck(){
  if(numRow < 3){
    Serial.println("numRow must be greater than 3! Please change and reupload.");
    while(true);
  }
  if(numSolution < 2){
    Serial.println("numSolution must be greater than 1! Please change and reupload.");
    while(true);
  }
  if(numSolution > numRow){
    Serial.println("numRow must be greater or equal to numSolution! Please change and reupload.");
    while(true);
  }
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
  byte elimSwitches[numRow];       // Elimination sort array to track remaining switch indexes
  bool countLights[lightCombos];      // Count array to track light pattern occurences in solution
  for (int i=0; i<numRow; i++){    // For each possible switch...
    elimSwitches[i] = i;              // Initialize elimination sort for selecting correct switches
  }
  for(int i=0; i<lightCombos; i++){   // For each possible light pattern...
    countLights[i] = 0;               // Initialize occurences to 0
  }
  for (int i=0; i<numSolution; i++){  // For each switch and light pattern solution...
    do {
      solutionSwitches[i] = random(0,numRow); // Generate a random index of a switch
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
-Store number of switches, numRow
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

To create a solution for all on, we can use the fact that XOR will always produce a 1 on an odd number of 1s.
We will create a set of odd numbers between 1 and arrSize. We will generate odd numbers, and store them as
a number that, in boolean, represents the number of 1 bools that coorespond to the odd number generated. Store
that occurence in count array. For each occurence there will be x
-i=0; i<arrSize; i++...
  randNum = random(1,arrSize+1) | 0x0001, random number within number of rows, ensure odd
  countArr[randNum]++;
  -j=1; j<randNum; j++...
    -bit shifting to perform the operation 2^0 + 2^1 + ... + 2^j
    odds[i] <<= 1, shift bit left
    odds[i] |= 0x0001, set first bit to 1
0 0 1 1 1
0 0 1 1 1
0 0 1 1 1
0 0 0 0 1
1 1 1 1 1

x5,y3,r10  x4,y3,r4 x3,y3,r1
0 0 1 1 1  0 1 1 1  1 1 1
0 1 1 1 0  1 0 1 1
0 1 1 0 1  1 1 0 1
0 1 0 1 1  1 1 1 0
1 0 0 1 1  
1 0 1 0 1
1 0 1 1 0
1 1 0 0 1
1 1 0 1 0
1 1 1 0 0

x4,y2,r6   x3,y2,r3
0 0 1 1     0 1 1
0 1 0 1     1 1 0
1 0 0 1     1 0 1
0 1 1 0    
1 0 1 0
1 1 0 0
 */
 
void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

}
