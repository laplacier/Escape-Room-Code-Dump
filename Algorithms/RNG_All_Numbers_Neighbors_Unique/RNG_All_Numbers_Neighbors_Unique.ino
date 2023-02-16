/*
 * ALL NUMBERS WITH UNIQUE NEIGHBORS ALGORITHM
 * 
 * Inputs:
 *   - Total unique numbers from 0 to n.
 *   - Length of the pattern to be generated.
 * 
 * This algorithm will generate a random assortment of numbers with the following
 * conditions:
 *   - The pattern will consist of all unique numbers     
 *   - For each number in the pattern, every neighbor will be unique
 * 
 * Examples:
 *   - Inputs:
 *     - Total unique numbers: 5
 *     - Length of pattern: 6
 *   - Patterns:
 *     - 1 2 3 4 5 1 - Valid, all numbers used, all neighbors unique 
 *     - 1 1 2 3 4 5 - Invalid, all numbers used, neighbors NOT unique (1 1)
 *     - 1 5 4 2 3 2 - Valid, all numbers used, all neighbors unique
 *     - 1 3 2 4 1 3 - Invalid, numbers NOT used (5), all neighbors unique
 * 
 * This algorithm is useful for:
 *   - Generating a solution to a puzzle that interacts with every element
 *     at least once.
 *   - Generating a solution with no repeat inputs, to ensure appropriate
 *     complexity for a randomly generated puzzle.
 *     
 * by Adam Billingsley
 * created 16 Feb, 2023
 */


/*==========================GLOBAL SETTINGS============================//
 * REQUIREMENTS:
 *   - uniqueNum must be less than or equal to patternLength
 *   - uniqueNum and patternLength must be positive and nonzero
 *   - uniqueNum cannot exceed 255. This is an artificial limit chosen
 *     for practical application. This algorithm can, if repreogrammed, 
 *     extend to infinite numbers and lengths.
 */
#define uniqueNum     11 // Number of unique numbers from 0 to uniqueNum-1
#define patternLength 20 // Length of generated pattern

byte elimArr[uniqueNum]; // Stores the pattern data for an elimination sort
byte elimLower;          // Lower bound for elimination sort
byte elimUpper;          // Upper bound for elimination sort
byte numToGo;            // Number of numbers left to generate for pattern
byte solution[patternLength]; // Stores valid pattern

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
 * Function: shiftInRight
 * This function accepts an array, the number to insert, the position 
 * to insert the number, and the upper bound of the shift. Shifts the 
 * array right from these two bounds. Does not resize the input array 
 * to increase speed; user must handle ignoring the unnecessary values.
 * 
 * Example: patternLength - numToGo - 1 = 5, position = 3, insert = 3
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
 *  - Repeat until i > patternLength - numToGo
 *  Index: 0  1  2  3  4  5  6  7  8
 *  Value: 1  3  5  3  7  9  4  0  0
 *  
 *  - Increment Upper bound by 1 and exit.
 */
void shiftInRight(byte insert, byte pos, byte limit = patternLength-numToGo){
  byte temp;
  for(int i=pos; i<=limit; i++){ // Starting at index=pos, for each index of the array that has a pattern value...
    temp = solution[i];          // Store current number in a temp var
    solution[i] = insert;        // Insert the shifted number into the pattern
    insert = temp;               // Prepare the temp number to be inserted on next loop
  }
}

/*
 * Function: elimSort
 * This function accepts an elimination sort array, a lower bound, and an
 * upper bound. Generates a random number between the lower and upper bound and performs a single elimination sort. Returns a post processed
 * elimination sort array and new lower and upper bounds.
 * 
 * An elimination sort is effective for speed when:
 *  - Attempting to find a unique number.
 *  - The number of unique values is large.
 * 
 * In a traditional loop to check each previous number, we can be checking up 
 * to n^2 numbers for each number, with no guarantee of a unique value. In an 
 * elimination sort, no matter how many unique numbers there are, a generation 
 * of a unique number in an elimination sort is a single array index check with 
 * a guaranteed unique value.
 * 
 * Time complexity of:
 *   - Traditional sort - O(n^2)
 *   - Elimination sort - O(n + n/2) = O(n)
 * Space complexity of:
 *   - Traditional sort - O(n)
 *   - Elimination sort - O(2n) = O(n)
 * 
 * However, an elimination sort, by nature, does not create duplicate numbers. 
 * This method cannot be used for generating an amount of numbers greater than 
 * the number of unique numbers.
 * 
 * In our all numbers neighbors unique example, this method is used whenever 
 * it is applicable due to its speed.
 */
byte elimSort(){
  byte pos = random(elimLower, elimUpper+1);              // Randomly select an index of a number to pull for an elimination sort
  byte num = elimArr[pos];                                // Store that number
  if(elimArr[pos - elimLower] < elimArr[elimUpper - pos]) // If that number is closer to the left side of the elimination array...
    shiftOutRight(pos);                                   // Shift elimination array right
  else 
    shiftOutLeft(pos);                                    // Otherwise, shift elimination array left
  return num;                                             // Return the number shifted out of the elimination array
}

/*
 * Function: generatePattern 
 * This function generates a random all numbers, neighbors unique pattern.
 * It utilizes elimination sorting to ensure all unique numbers are present
 * in the pattern. Afterwards, it generates random numbers from 0 to 
 * uniqueNum-1 and attempts to insert them into a random location in the
 * pattern, following the unique neighbor rule. It does this until the
 * desired pattern length has been reached.
 */

void generatePattern(){
  elimUpper = uniqueNum-1;                                            // Initialize upper bound of elimination sort
  elimLower = 0;                                                      // Initialize lower bound of elimination sort
  numToGo = patternLength;                                            // Stores number of numbers left to generate in pattern
  for(byte i=0; i<uniqueNum; i++)                                     // For each index in the elimination sort array...
    elimArr[i] = i;                                                   // Initialize the array with every unique number
    
  for(byte i=0; i<uniqueNum; i++){                                    // For each unique number...
    solution[i] = elimSort();                                         // Perform an elimination sort and store the selected number
    numToGo--;                                                        // Denote one less number left to generate
  }
  while(numToGo){                                                     // If we still have numbers to generate...
    byte randNum = random(0, uniqueNum);                              // Generate a random number from 0 to uniqueNum - 1
    byte pos = random(0, patternLength - numToGo);                    // Generate a random position within the bounds of generated numbers
    bool flag_inserted = false;                                       // Flag checks if number has been inserted into pattern

    while(!flag_inserted){                                                // While the number has not been inserted into the pattern...
      //NICHE CASES
      if(pos == patternLength - numToGo - 1 && randNum != solution[pos]){ // If the new position is at the final generated number and is a valid insertion...
        shiftInRight(randNum, pos+1, pos+1);                              // Insert number at the end
        flag_inserted = true;
      }
      else if(pos > patternLength - numToGo - 1 && randNum != solution[0]){ // If the new position is outside the bounds of generated numbers and can be inserted at the beginning...
        shiftInRight(randNum, 0);                                           // Insert number at beginning
        flag_inserted = true;
      }
      else if(pos > patternLength - numToGo && randNum == solution[0]){ // Otherwise, if the new position is outside the bounds of generated numbers and the first number is an identical neighbor...
        pos = 1;                                                        // Move the position to +1 after the beginning  
      }
      //STANDARD CASES
      else if(randNum == solution[pos] || randNum == solution[pos+1]){  // If number has an identical neighbor...
        pos += 2;                                                       // Increment the position to check the next pair of neighbors
      }
      else{                                                             // If number has unique neighbors...
        shiftInRight(randNum, pos+1);                                   // Insert the number between them
        flag_inserted = true;
      }
    }
    numToGo--; // One less number to generate
  }
}

void setup() {
  Serial.begin(19200);
  randomSeed(analogRead(A0));     // Use noise fluxuations from the A0 pin to seed the RNG
  for(byte i=0; i<uniqueNum; i++) // For each index in the elimination sort array...
    elimArr[i] = i;               // Initialize the array with every unique number

  generatePattern();
  Serial.println();
  for(byte i=0; i<patternLength; i++){
    Serial.print(solution[i]);
    Serial.print(", ");
  }
}

void loop() {

}
