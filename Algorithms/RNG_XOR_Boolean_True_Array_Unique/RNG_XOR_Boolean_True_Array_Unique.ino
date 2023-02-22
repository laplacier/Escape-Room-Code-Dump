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
 * Common bit shifting shortcuts you will see in this program:
 *                n >> m = Divide n by 2^m, rounded down
 *                n << m = Multiply n by 2^m
 * (n & 0x01) + (n >> m) = Divide n by 2^m, rounded up
 * (n & 0x01) + (n >> 1) = The number of odd numbers from 0 to n
 *       (n << 1) | 0x01 = The nth odd number
 *
 * by Adam Billingsley
 * created  20 Feb, 2023
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
#define numSolution 7  // Number of arrays in the unique solution
#define numRow      8  // Total number of arrays
#define arrSize     9  // Total number of boolean vals in an array

const byte lastRow = numRow % 8;     // Amount of arrays to create in the last row chunk
const byte lastCol = arrSize % 8;    // Amount of bools to create in the last column chunk
const int rowChunk = numRow >> 3 + (lastRow > 0);    // (numRow/8) Determine how many rows of chunks to make
const int colChunk = arrSize >> 3 + (lastCol > 0);   // (arrSize/8) Determine how many columns of chunks to create
byte pattern[colChunk][rowChunk][8]; // Stores valid pattern
int solution[numSolution]; // Stores the index of the solution arrays
byte chunkPattern[8];

/*
 * Function: shiftOutLeft
 * This function accepts a reservoir sampling array, the position to start 
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
void shiftOutLeft(int arr[], int *upper, int pos){
  for(int i=pos; i<*upper; i++){ // Starting at index=pos, for each index of the array to the upper bound...
    arr[i] = arr[i+1];      // Shift index on the right to the current index.
  }
  (*upper)--;                      // Decrement upper bound by 1 to ignore useless data
}

/*
 * Function: shiftOutRight
 * This function accepts a reservoir sampling array, the position to finish 
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
void shiftOutRight(int arr[], int *lower, int pos){
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
//void shiftInRight(int insert, int pos, int upper){
//  int temp;
//  for(int i=pos; i<=upper; i++){ // Starting at index=pos, for each index of the array that has a pattern value...
//    temp = pattern[i];            // Store current number in a temp var
//    pattern[i] = insert;          // Insert the shifted number into the pattern
//    insert = temp;                // Prepare the temp number to be inserted on next loop
//  }
//}

/*
 * Function: resSort
 * This function accepts a reservoir sampling array, a lower bound, and an
 * upper bound. Generates a random number between the lower and upper bound 
 * and performs a single reservoir sample. Returns a post processed reservoir 
 * sampling array and new lower and upper bounds.
 * 
 * reservoir sampling is effective for speed when:
 *  - Attempting to find a unique number.
 *  - The number of unique values is large.
 * 
 * In a traditional loop to check each previous number, we can be checking up 
 * to n^2 numbers for each number, with no guarantee of a unique value. In 
 * reservoir sampling, no matter how many unique numbers there are, a generation 
 * of a unique number in reservoir sampling is a single array index check with 
 * a guaranteed unique value.
 * 
 * Time complexity of:
 *   - Traditional sort - O(n^2)
 *   - Reservoir sampling - O(n + n/2) = O(n)
 * Space complexity of:
 *   - Traditional sort - O(n)
 *   - Reservoir sampling - O(2n) = O(n)
 * 
 * However, reservoir sampling, by nature, does not create duplicate numbers. 
 * This method cannot be used for generating an amount of numbers greater than 
 * the number of unique numbers.
 * 
 * In our quest to insert dummy arrays into our unique solution, we will have to
 * perform other methods. However, this significantly speeds up the unique
 * generation portion.
 */
int resSort(int arr[], int pos, int *upper, int *lower){
  int num = arr[pos];                      // Store that number
  if(arr[pos-(*lower)] < arr[(*upper)-pos]) // If that number is closer to the left side of the reservoir sampling array...
    shiftOutRight(arr, lower, pos);         // Shift reservoir sampling array right
  else 
    shiftOutLeft(arr, upper, pos);          // Otherwise, shift reservoir sampling array left
  return num;                               // Return the number shifted out of the reservoir sampling array
}

/*
 * Function: nCrScrambler
 * Accepts the desires number of ones, length of the array,
 * and number of iterations to perform in the array. Outputs an
 * array of ones after performing iterations. This routine is
 * capable of iterating through every single possible nCr combo
 * possible for the provided parameters.
 */
byte nCrScrambler(byte ones, byte arrLength, byte iterations){
  /*******************************************************/
  /*******\DETERMINE HOW MANY PLACES TO SHIFT ONES/*******/
  /*******************************************************/
  byte shifts[ones]; // Stores the number of shifts to perform
  for(byte i=0; i<ones; i++){
    shifts[i] = 0;
  }
  while(iterations > 0){                     // If we aren't out of iterations to make...
    byte window = ones-1;                    // Start at the least significant 1
    while(window > 0 && iterations > 0){     // While we haven't reached the last 1 in the array and have iterations left to make...
      if(shifts[window] < shifts[window-1]){ // If this 1 has shifted less than the 1 on the left...
        shifts[window]++;                    // Shift the 1
        for(byte i=window+1; i<ones; i++){   // For each less significant 1...
          shifts[i] = 0;                     // Send them back to the beginning
        }
        iterations--;                        // Denote an iteration taking place
        window = ones-1;                     // Return to the least significant 1
      } else{
        window--;                            // Look at the next 1
      }
    }
    if(iterations > 0){                      // If we aren't out of iterations to make...
      if(shifts[0] != (arrLength - ones)){   // If there is space left to shift...
        shifts[0]++;                         // Shift the most significant 1 to the left
        for(byte i=1; i<ones; i++){          // For each less significant 1...
          shifts[i] = 0;                     // Send them back to the beginning
        }
      }
      iterations--;                          // Denote an iteration taking place
    }
  }

  /*******************************************************/
  /***********\CREATE AND RETURN SHIFTED ARRAY/***********/
  /*******************************************************/
  byte result = 0;
  for(byte i=0; i<ones; i++){ // For each 1 in the array...
    //Serial.print("[");
    //Serial.print(shifts[i]);
    //Serial.print("] ");
    byte temp = 1;                // Set bit to 1
    temp <<= (shifts[i]+(ones - 1 - i));      // Shift the 1 to the left by the calculated amount
    result |= temp;              // Add the 1 to the accumulating result
  }
  //Serial.println();
  return result;
}

/*
 * Function: nCr
 * Takes the globally defined number of ones and total array length. Calculates 
 * the total number of combinations that can be made in the boolean array
 * consisting of exactly the inputted number of ones.
 * 
 * The total number of unique combinations of a boolean array follows the nCr 
 * combinations formula which is the following:
 * 
 *        n! / (k!(n - k)!)
 *        
 * Where n is the length of the array and k is the number of ones. However, we can
 * further simplify this formula for computational speed by reducing the scope to
 * analylizing only the amount of zeroes and ones. By extracting these two values,
 * we can obtain a much faster formula:
 * 
 *        for(int i=i; i<=k; i++){ 
 *          combos *= r + i;  
 *          denominator *= i; 
 *        }
 *        combos /= denominator;
 * 
 * Where r and k are the number of zeroes and ones and can be used interchangeably.
 * The fastest speed will be obtained by making k the smallest value between the two.
 */
byte nCr(byte ones, byte arrLength){
  byte combos = 1;                // Store combos. By default, combos = 1 when arrLength = ones.
  byte denominator = 1;           // Store denominator which stores the fractorial to divide    
  byte r;
  byte k;
  
  if(arrLength - ones > ones){   // If there are more zeroes...
    r = arrLength - ones;
    k = ones;                    // Make k = ones
  } else{                        // Otherwise...
    r = ones;
    k = arrLength - ones;        // Make k = zeroes
  }
  for(byte i=1; i<=k; i++){       // For each zero or one, whichever was smaller...
    combos *= r + i;             // Perform a fractorial operation k times, where r is constant
    denominator *= i;            // Perform a fractoral operation
  }
  combos = combos / denominator; // Divide to produce the total combinations
  
  return combos;                 // Return the total combinations
}

/* 
 * Function: isSolutionValid 
 * Takes a generated solution array and the array length as input.
 * 
 * Determines if the solution produced reasonable arrays. The constraints are:
 *  - No two solution arrays are identical
 *  - No solution array consists of all zeroes or ones
 * We can determine this by performing an XOR of every switch. If 0 or max, one
 * of these conditions is true and we can flag the solution for additional bool
 * shuffling.
 */
bool isSolutionValid(byte arr[], byte arrLength){
  for(byte i=1; i<arrLength; i++){                      // For each number of arrays in the solution to XOR, aside from all of them...
    byte wrongCombos = nCr(i, arrLength);                // Determine the number of times each array can be combined uniquely
    for(byte j=0; j<wrongCombos; j++){                   // For the number times we can combine each array...
      byte wrongArray = 0;                              // Stores resulting wrong array
      byte arraysToXOR = nCrScrambler(i, arrLength, j); // Determine the array combo to XOR
      for(byte k=0; k<arrLength; k++){                  // For each array in the solution...
        if(bitRead(arraysToXOR, k)){                    // If we need to XOR this array...
          wrongArray ^= arr[k];                         // XOR this array
        }
      }
      if(!wrongArray){ // If we notice two duplicate arrays in the solution or an array of all 1s or 0s...
        return 0;      // Return flag to retry, we made a bad solution...
      }
    }
  }
  return 1;            // Return flag to continue, solution is valid
}

/* 
 * Function: generateChunk
 * Takes the globally defined number of switches and number of
 * desired correct switches. Generates an array of correct switches
 * and an array of their light patterns.
 * 
 * To create a solution for all on, we can use the fact that XOR will always produce a 1 on an odd number of 1s.
 * We will create a set of odd numbers between 1 and numRow. We will generate odd numbers, and store them as
 * a number that, in boolean, represents the number of 1 bools that coorespond to the odd number generated. Store
 * that occurence in count array. For each occurence there will be x
 * 
 * The total number of permutations of an array will be:
 * 
 *        n! / (k!(n - k)!)
 *        
 * Where n is the length of the array and k is the number of ones
 * 
 * If we are generating odd arrays and the number of correct arrays is less than 8, the remainder should be legit dummies
 * If we are generating even arrays, the number of correct arrays MUST be 0 we do nto need to generate dummy arrays when generating evens.
 */
bool generateChunk(byte row, byte col, bool genOdds = 0){
  int nCrArr[row];                             // Temporary array to store nCr results
  byte numOnes = (row >> 1);           // Initialize number of ones to even occurences
  if(genOdds){                                       // If this chunk should produce an odd number of ones...
    numOnes += (row & 0x01);               // Adjust the number of ones to accomodate the number of odd occurences

  }
  //Serial.print("numOnes:");
  //Serial.println(numOnes);
  byte countOnes[numOnes];                            // Stores the total occurence of each odd/even number in the solution
  int combos[numOnes];                               // Stores the total combinations of every number of ones within the number of columns
  byte numToGo = col;                          // Number of arrays left to generate during reservoir sampling
  for(byte i=0; i<numOnes; i++){                     // For every odd/even amount of ones possible...
    countOnes[i] = 0;                                // Initialize count occurences to 0. Necessary for repeat calls to function.
    if(genOdds){
      combos[i] = nCr((i << 1) | 0x01, row); // (2*i)+1 Calculate the number of odd combinations possible for all ones within the number of columns
    } else{
      combos[i] = nCr((i+1) << 1, row); // 2*(i+1) Calculate the number of even combinations possible for all ones within the number of columns
    }
  }
  for(byte i=0; i<col; i++){     // For each array to generate...
    countOnes[random(0, numOnes)]++; // Generate a random number of odd/even ones within the number of columns and increment its occurence
  }
  for(byte i=0; i<numOnes; i++){     // For each potential odd/even number of ones...
    int resArr[combos[i]];         // Stores the pattern data for reservoir sampling
    int combosLeft = combos[i];    // Number of unique combinations remaining
    int resLower = 0;              // Lower bound for reservoir sampling
    int resUpper = combos[i]-1;    // Upper bound for reservoir sampling
    for(byte k=0; k<combos[i]; k++) // For each possible combination of selected number of ones...
      resArr[k] = k;                // Initialize each index in the reservoir sampling array with every unique number
    for(byte j=0; j<countOnes[i]; j++){  // For each occurence of that odd number...
      if(!combosLeft){                  // If we ran out of unique combos to make...
        combosLeft = combos[i];         // Reinitialize unique combos remaining
        resLower = 0;                   // Reinitialize lower bound for reservoir sampling
        resUpper = combos[i]-1;         // Reinitialize upper bound for reservoir sampling
        for(int k=0; k<combos[i]; k++) // For each possible combination of selected odd number...
          resArr[k] = k;                // Initialize each index in the reservoir sampling array with every unique number
      }
      int randCombo = random(resLower,resUpper+1);                                  // Generate a random number of iterations within the unique number of iterations left
      //Serial.print("Lower:");
      //Serial.print(resLower);
      //Serial.print(" Upper:");
      //Serial.print(resUpper);
      //Serial.print(" Iterations chosen:");
      //Serial.println(randCombo);
      randCombo = resSort(resArr, randCombo, &resUpper, &resLower);                  // Perform a reservoir sample to store and eliminate the number.
      combosLeft--;                                                                  // Decrement the possible combinations by 1, since one was used and eliminated
      //Serial.print(" Result for ");
      //Serial.print((i << 1) | 0x0001);
      //Serial.print("ones:");
      if(genOdds){
        nCrArr[col-numToGo] = nCrScrambler((i << 1) | 0x01, row, randCombo); // Generate a valid odd array with the random interation value and store the array in the solution
      } else{
        nCrArr[col-numToGo] = nCrScrambler((i+1) << 1, row, randCombo); // Generate a valid even array with the random interation value and store the array in the solution
      }
      numToGo--;
    }
  }
  // Scramble the order of these arrays up with another reservoir sampling
  int resArr[col];        // Stores each nCr result for reservoir sampling
  int resLower = 0;             // Lower bound for reservoir sampling
  int resUpper = col-1;     // Upper bound for reservoir sampling
  for(byte i=0; i<col; i++){ // For each nCr result...
    resArr[i] = nCrArr[i];       // Store that result in the reservoir sampling array
  }
  for(byte i=0; i<row; i++){                                  // For each nCr result...
    int randCombo = random(resLower,resUpper+1);                 // Generate a unique position to grab a number from
    for(int j=0; j<row; j++){
      //Serial.print(bitRead(nCrArr[i],j));
      //Serial.print(" ");
    }
    nCrArr[i] = resSort(resArr, randCombo, &resUpper, &resLower); // Perform a reservoir sample to store and eliminate the number.
    //Serial.println();
  }
  //Serial.println();
  // Transpose the array to get our numbers....
  for(byte i=0; i<col; i++){ // For each boolean val in the array
    for(byte j=0; j<row; j++){ // For each solution array
      bitWrite(chunkPattern[j], i, bitRead(nCrArr[i],j)); // Store the jth bool of the ith nCr array in the ith bool of the jth solution
    }
  }

  if(!isSolutionValid(chunkPattern, row)){ // If the solution has duplicates...
    return 1;                                        // Bad solution, flag for a redo
  }
  if(genOdds){ // We ONLY need to perform this during odd generation (correct arrays).
    // Determine which dummy switches cannot be allowed to generate
    byte dummyCombos = (1 << col) - 1; // Total possible arrays
    bool resCount[dummyCombos];            // Stores every occurence of an array during reservoir sampling
    int resDummy[dummyCombos];            // Stores all possible unique dummy arrays that will not produce a solution
    resLower = 1;                          // Lower bound for reservoir sampling. Prefilter all 0's
    resUpper = dummyCombos-2;              // Upper bound for reservoir sampling. Prefilter all 1's
    for(byte i=0; i<dummyCombos; i++){ // For each possible array...
      resCount[i] = 0;                 // Initialize array count occurences to 0. Necessary for repeat calls to function.
      resDummy[i] = i;                 // Store that array for reservoir sampling
    }

    for(byte i=0; i<row; i++){                      // For each correct array...
      solution[i] = i;                                     // Initialize the positions of the solution arrays
      resSort(resDummy, chunkPattern[i], &resUpper, &resLower); // Perform a reservoir sample to eliminate the number.
      resCount[chunkPattern[i]] = 1;                            // Flag this number for being removed as a dummy option
    }
    for(byte i=1; i<row; i++){                      // For each number of arrays in the solution to XOR, aside from all of them...
      int wrongCombos = nCr(i, row);              // Determine the number of times each array can be combined uniquely
      for(int j=0; j<wrongCombos; j++){                    // For the number times we can combine each array...
        int wrongArray = 0;                               // Stores resulting wrong array
        byte arraysToXOR = nCrScrambler(i, row, j); // Determine the array combo to XOR
        for(byte k=0; k<row; k++){                  // For each array in the solution...
          if(bitRead(arraysToXOR, k)){                     // If we need to XOR this array...
            wrongArray ^= chunkPattern[k];                      // XOR this array
          }
        }
        if(!resCount[wrongArray]){                             // If this array was not already determined to be wrong...
          resSort(resDummy, wrongArray, &resUpper, &resLower); // Perform a reservoir sample to eliminate the number.
          resCount[wrongArray] = 1;                            // Flag this number for being removed as an option
        }
      }
    }

    // FINALLY, we can produce proper dummy arrays
    for(byte i=row; i<col; i++){                          // For the number of dummy arrays to produce...
      int randArr = random(resLower,resUpper+1);                          // Generate a random position to pull a dummy array from the dummy reservoir
      chunkPattern[i] = resSort(resDummy, randArr, &resUpper, &resLower); // Perform a reservoir sample to generate a unique, valid dummy array
    }
  }

  return 0;
}

/* 
 * Function: generateSolution
 * Takes the globally defined number of switches and number of
 * desired correct switches. Generates an array of correct switches
 * and an array of their light patterns.
 * 
 * To create a solution for all on, we can use the fact that XOR will always produce a 1 on an odd number of 1s.
 * We will create a set of odd numbers between 1 and numRow. We will generate odd numbers, and store them as
 * a number that, in boolean, represents the number of 1 bools that coorespond to the odd number generated. Store
 * that occurence in count array. For each occurence there will be x
 * 
 * The total number of permutations of an array will be:
 * 
 *        n! / (k!(n - k)!)
 *        
 * Where n is the length of the array and k is the number of ones
 * 
 * max 8x8x8 arr, split into array chunks
 * arrSize, number of booleans, rows of initial program, uniquely find each combo of 8 as well as remainder
 * numSolution, cols of initial program, find the first 8 odd, then every proceeding combination MUST BE EVEN. perhaps a bool toggle?
 */
bool generateSolution(){
  //Determine the number of chunks to produce and their location
  int correctToGo = numSolution; // Store the number of correct arrays to generate to count
  for(int i=0; i<rowChunk; i++){ // For each row chunk to create (vertical)...
    byte rows = 8;
    if((correctToGo >> 3) < 8){ // If we have less than 8 solutions to generate...
      rows = correctToGo; // Make the odds generated equal to the last bit of solution arrays
    }
    if(i == rowChunk - 1 && lastRow){ //If this is the last row chunk to create and we have an incomplete chunk to make...
      rows = lastRow; // Resize the row dimension of the chunk to the number of arrays in the last row
    }
    for(int j=0; j<colChunk; j++){ // For each column chunk to create (horizontal)...
      byte columns = 8;
      if(j == colChunk - 1 && lastCol){ // If this is the last column chunk to create and we have an incomplete chunk to make...
        columns = lastCol;              // Resize the column dimension of the chunk to the amount of bools in the last column
      }
      bool flag_redo; // Flag to trigger a regeneration
      bool makeOdd = false; // Flag to determine if we generate even or odd arrays
      if(correctToGo){ // If we still have correct arrays to generate...
        makeOdd = true; // Trigger odd array generation
      }
      flag_redo = generateChunk(rows, columns, true);
      while(flag_redo){
        Serial.println("I made a bad chunk. Let's try again...");
        flag_redo = generateChunk(rows, columns, true);
      }
      for(byte k=0; k<8; k++)
        pattern[i][j][k] = chunkPattern[k];
    }
    correctToGo -= rows; // Check off the number of correct array generated
  }
}
  /*// FINALLY, we can produce proper dummy arrays
  for(byte i=0; i<numRow - numSolution; i++){                       // For the number of dummy arrays to produce...
    int patternUpper = i + numSolution;                            // Upper bound of the current pattern array
    int randArr = random(resLower,resUpper+1);                    // Generate a random position to pull a dummy array from the dummy reservoir
    int randPos = random(0,patternUpper+1);                       // Generate a random position in the pattern to place the dummy array
    int dummy = resSort(resDummy, randArr, &resUpper, &resLower); // Perform a reservoir sample to generate a unique, valid dummy array
    shiftInRight(dummy, randPos, patternUpper);                    // Insert the dummy into the pattern
    // Update the position of the solution arrays
    for(byte j=0; j<numSolution; j++){  // For each solution array...
      if(solution[j] >= randPos){      // If the inserted dummy array shifted the current solution array over...
        solution[j]++;                 // Update the position of the solution array
      }
    }
  }
  return 0;
}*/

void printPuzzle(){
  Serial.println("The arrays generated, top to bottom, are...");
  Serial.print("   ,");
  int rowBits = (colChunk*8) + (arrSize%8);
  Serial.print(rowBits);
  for(int i=0; i<rowBits; i++){
    Serial.print("___");
  }
  Serial.println();
  for(int i=0; i<rowChunk; i++){ //(numRow%8)
    for(int j=0; j<8; j++){ // (arrSize%8)
      if(j+(i*8) < 10){
        Serial.print("0");
      }
      if(j+(i*8) < 100){
        Serial.print("0");
      }
      Serial.print(j+(i*8));
      Serial.print("|");
      for(int k=0; k<colChunk; k++){
        for(int m=0; m<8; m++){
          Serial.print(" ");
          Serial.print(bitRead(pattern[i][k][j],m));
          Serial.print(" ");
        }
      }
      Serial.println();
    }
    Serial.println();
  }
  Serial.print("The solution arrays are at indexes: ");
  for(int i=0; i<numSolution; i++){ // For each solution array..
    Serial.print(solution[i]);
    Serial.print(" ");
  }
  Serial.println();
}

void printChunk(){
  Serial.println("The arrays generated, top to bottom, are...");
  Serial.print("  ,");
  for(int i=0; i<arrSize; i++){
    Serial.print("___");
  }
  Serial.println();
  for(int i=0; i<8; i++){
    if(i < 10){
      Serial.print("0");
    }
    Serial.print(i);
    Serial.print("|");
    for(int j=0; j<arrSize; j++){
      Serial.print(" ");
      Serial.print(bitRead(chunkPattern[i],j));
      Serial.print(" ");
    }
    Serial.println();
  }
}

void setup() {
  Serial.begin(19200);
  Serial.println();
  randomSeed(analogRead(A0));          // Use noise fluxuations from the A0 pin to seed the RNG
  //bool flag = generateSolution();
  generateSolution();
  // Print our solution
  //Serial.println(pattern[0][0][0]);
  //printChunk();
  printPuzzle();
}

void loop() {
}
