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
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define DEBUG false
#define numSolution 8  // Number of arrays in the unique solution
#define numRow      8  // Total number of arrays
#define arrSize     13  // Total number of boolean vals in an array

const unsigned char lastRow = numRow % 8;                      // Amount of arrays to create in the last row chunk
const unsigned char lastCol = arrSize % 8;                     // Amount of bools to create in the last column chunk
const unsigned char rowChunk = (numRow >> 3) + (lastRow > 0);  // (numRow/8) Determine how many rows of chunks to make
const unsigned char colChunk = (arrSize >> 3) + (lastCol > 0); // (arrSize/8) Determine how many columns of chunks to create
unsigned char pattern[rowChunk][colChunk][8];                  // Stores:  Valid pattern
int solution[numSolution];                            //          Bool index location of the solution arrays
unsigned char chunkPattern[8];                                 //          The 8x8 bool chunk for each chunk generation
bool dummyCount[colChunk][255];                       //          The valid and invalid dummy arrays that can be generated 

/* a=target variable, b=bit number to act upon 0-n */
#define BIT_SET(a,b) ((a) |= (1ULL<<(b)))
#define BIT_CLEAR(a,b) ((a) &= ~(1ULL<<(b)))
#define BIT_FLIP(a,b) ((a) ^= (1ULL<<(b)))
#define BIT_READ(a,b) (!!((a) & (1ULL<<(b))))        // '!!' to make sure this returns 0 or 1
#define BIT_WRITE(a,b,x) ((a) = (a & ~(1ULL<<b))|(x<<b))

#define ODD_NUM(n) (((n-1)<<1ULL)|1ULL) // The nth odd number (1,3,5,etc...)
#define EVEN_NUM(n) ((i+1)<<1ULL) // The nth even num (2,4,6,etc...)
#define NUM_ODD(n) ((n&1ULL)+(n>>1)) // The number of odd numbers from 1 to n
#define NUM_EVEN(n) ((n<<1ULL)) // The number of even numbers from 0 to n
#define MULTIPLY_POW2(n,m) (n<<m) // Multiply n by 2^m
#define DIVIDE_POW2D(n,m) (n>>m) // Divide n by 2^m, rounded down
#define DIVIDE_POW2U(n,m) ((n>>m)+(n&1ULL)) // Divide n by 2^m, rounded up

#define random(min,max) ((rand()%(max-min))+min) // Emulates Arduino implementation of RNG where a range between min and max-1 is generated

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
  int num = arr[pos];                       // Store that number
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
unsigned long nCrScrambler(int ones, int arrLength, unsigned long iterations){
  /*******************************************************/
  /*******\DETERMINE HOW MANY PLACES TO SHIFT ONES/*******/
  /*******************************************************/
  int shifts[ones]; // Stores the number of shifts to perform
  for(int i=0; i<ones; i++){
    shifts[i] = 0;
  }
  while(iterations > 0){                     // If we aren't out of iterations to make...
    int window = ones-1;                    // Start at the least significant 1
    while(window > 0 && iterations > 0){     // While we haven't reached the last 1 in the array and have iterations left to make...
      if(shifts[window] < shifts[window-1]){ // If this 1 has shifted less than the 1 on the left...
        shifts[window]++;                    // Shift the 1
        for(int i=window+1; i<ones; i++){   // For each less significant 1...
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
        for(int i=1; i<ones; i++){          // For each less significant 1...
          shifts[i] = 0;                     // Send them back to the beginning
        }
      }
      iterations--;                          // Denote an iteration taking place
    }
  }

  /*******************************************************/
  /***********\CREATE AND RETURN SHIFTED ARRAY/***********/
  /*******************************************************/
  unsigned long result = 0;
  for(int i=0; i<ones; i++){ // For each 1 in the array...
    unsigned long temp = 1;                // Set bit to 1
    temp <<= (shifts[i]+(ones - 1 - i));      // Shift the 1 to the left by the calculated amount
    result |= temp;              // Add the 1 to the accumulating result
  }
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
unsigned long nCr(unsigned char ones, unsigned char arrLength){
  unsigned long combos = 1;      // Store combos. By default, combos = 1 when arrLength = ones.
  unsigned long denominator = 1; // Store denominator which stores the fractorial to divide    
  unsigned char r;               // The largest amount between zeroes and ones
  unsigned char k;               // The smallest amount between zeroes and ones

  if(ones > arrLength){ // If it is impossible to make a valid combination...
    return 0;           // Return no combinations possible
  }
  
  if(arrLength - ones > ones){   // If there are more zeroes...
    r = arrLength - ones;
    k = ones;                    // Make k = ones
  } else{                        // Otherwise...
    r = ones;
    k = arrLength - ones;        // Make k = zeroes
  }
  for(int i=1; i<=k; i++){       // For each zero or one, whichever was smaller...
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
bool isSolutionValid(unsigned char arr[], unsigned char arrLength){
  for(unsigned char i=1; i<arrLength; i++){                      // For each number of arrays in the solution to XOR, aside from all of them...
    unsigned char wrongCombos = nCr(i, arrLength);                // Determine the number of times each array can be combined uniquely
    for(unsigned char j=0; j<wrongCombos; j++){                   // For the number times we can combine each array...
      unsigned char wrongArray = 0;                              // Stores resulting wrong array
      unsigned char arraysToXOR = nCrScrambler(i, arrLength, j); // Determine the array combo to XOR
      for(unsigned char k=0; k<arrLength; k++){                  // For each array in the solution...
        if(BIT_READ(arraysToXOR, k)){                    // If we need to XOR this array...
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
 * Function: findDummies
 * This function accepts a row x col bool chunk and determines which
 * unsigned char arrays to avoid using when generating dummy unsigned char arrays.
 * 
 * It XORs every potential combination against all ones to produce a
 * unsigned char array that will ruin the uniqueness of our solution. It then
 * flags these unsigned char arrays as invalid and modifies the global variable
 * dummyCount[colChunk][255] which is a counting sort array keeping track
 * of valid and invalid dummy array bytes for each row chunk in the column
 * chunk's column.
 * 
 * pattern[i][j][k]
 * XOR pattern[0][j][0] 0, 
 */
void findDummies(unsigned char col){
  // Determine which dummy switches cannot be allowed to generate
  dummyCount[col][255] = 1;                              // (2^col - 1) is never a valid array unsigned char
  dummyCount[col][0] = 1;                                // 0 is never a valid array unsigned char
  for(int i=1; i<numSolution; i++){                      // For the number of unique array bytes in the solution to XOR...
    printf("Combining %d array bytes...",i);
    unsigned long wrongCombos = nCr(i, numSolution);     // Determine the number of times each array unsigned char can be combined uniquely
    for(unsigned long j=0; j<wrongCombos; j++){          // For each unique array unsigned char combo of i array bytes...
      printf(".");
      unsigned long arraysToXOR = nCrScrambler(i, numSolution, j); // Determine the array bytes to XOR
      unsigned char wrongArray = 255;                             // Initialize wrong array unsigned char
      for(unsigned char k=0; k<rowChunk; k++){                    // For each row chunk of array bytes in the solution to XOR...
        unsigned char row = 8;                                    // Iterate through all 8 array bytes in the row chunk
        if(k == rowChunk - 1 && lastRow){                // But if we are iterating through the last row chunk and it isn't full...
          row = lastRow;                                 // Only iterate through the array bytes present in the last row chunk
        }
        for(unsigned char m=0; m<row; m++){                             // For each solution array unsigned char in the row chunk in the same column...
          int boolSelector = (k << 3) + m;                     // ((k * 8) + m) Calculate the index of the array unsigned char
          boolSelector = BIT_READ(arraysToXOR,boolSelector);   // Determine if we need to XOR this array unsigned char (is the bool 1?)
          if(boolSelector){                                    // If we need to XOR this array unsigned char...
            wrongArray ^= pattern[k][col][m];                  // XOR this array unsigned char to produce a dummy that would ruin a unique solution
          }
        }
      }
      dummyCount[col][wrongArray] = 1;                 // Flag the generated array unsigned char as a dummy to avoid
    }
    printf("\n");
  }
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
bool generateChunk(unsigned char row, unsigned char col, bool genOdds = 0){
  int nCrArr[8];             // Temporary array to store nCr results
  unsigned char numOnes = (row >> 1); // (row/2 rounded down) Initialize number of ones to even occurences
  if(genOdds){               // If this chunk should produce an odd number of ones...
    numOnes += (row & 0x01); // (row/2 returning the rounded num) Adjust the number of ones to accomodate the number of odd occurences
  }
  for(unsigned char i=0; i<8; i++){   // For each array...
    chunkPattern[i] = 0;     // Zero out the array
  }
  if(!numOnes){              // If we can't generate any ones...
    return 0;                // Do not execute tasks or flag for a regeneration
  }
  unsigned char countOnes[numOnes];                 // Stores the total occurence of each odd/even number in the solution
  unsigned char combos[numOnes];                    // Stores the total combinations for each valid number of ones within the number of columns
  unsigned char numToGo = col;                      // Number of arrays left to generate during reservoir sampling
  for(unsigned char i=0; i<numOnes; i++){           // For every odd/even amount of ones possible...
    countOnes[i] = 0;                      // Initialize count occurences to 0. Necessary for repeat calls to function.
    if(genOdds){                           // If we're generating an odd array...
      combos[i] = nCr(ODD_NUM(i+1), row);  // (2*i)+1 Calculate the number of odd combinations possible for all ones within the number of pre-transposed rows
    } else{                                // Otherwise, we're generating an even array...
      combos[i] = nCr(EVEN_NUM(i+1), row); // 2*(i+1) Calculate the number of even combinations possible for all ones within the number of pre-transposed rows
    }
  }
  for(unsigned char i=0; i<col; i++){     // For each array to generate...
    unsigned char ones = random(0, numOnes);
    countOnes[ones]++; // Generate a random number of odd/even ones within the number of columns and increment its occurence
  }
  for(int i=0; i<numOnes; i++){         // For each potential odd/even number of ones...
    int resArr[combos[i]];              // Initialize: Pattern data for reservoir sampling
    unsigned char combosLeft = combos[i];        //             Number of unique combinations remaining
    int resLower = 0;                   //             Lower bound for reservoir sampling
    int resUpper = combos[i]-1;         //             Upper bound for reservoir sampling
    for(unsigned char k=0; k<combos[i]; k++)     // For each possible combination of selected number of ones...
      resArr[k] = k;                    // Initialize each index in the reservoir sampling array with every unique number
    for(unsigned char j=0; j<countOnes[i]; j++){ // For each occurence of that odd number...
      if(!combosLeft){                  // If we ran out of unique combos to make...
        combosLeft = combos[i];         // Reinitialize: Unique combos remaining
        resLower = 0;                   //               Lower bound for reservoir sampling
        resUpper = combos[i]-1;         //               Upper bound for reservoir sampling
        for(unsigned char k=0; k<combos[i]; k++) //
          resArr[k] = k;                //               Each index in the reservoir sampling array with every unique number
      }
      int randCombo = random(resLower,resUpper+1);                           // Generate a random number of iterations within the unique number of iterations left
      randCombo = resSort(resArr, randCombo, &resUpper, &resLower);          // Perform a reservoir sample to store and eliminate the number.
      combosLeft--;                                                          // Decrement the possible combinations by 1, since one was used and eliminated
      if(genOdds){                                                           // If we're generating an odd array...
        nCrArr[col-numToGo] = nCrScrambler(ODD_NUM(i+1), row, randCombo); // Generate a valid odd array with the random interation value and store the array in the solution
      } else{                                                                // Otherwise, we're generating an even array...
        nCrArr[col-numToGo] = nCrScrambler(EVEN_NUM(i+1), row, randCombo);      // Generate a valid even array with the random interation value and store the array in the solution
      }
      numToGo--;                                                             // Decrement the number of combos left to generate
    }
  }

  // Scramble the order of these arrays up with another reservoir sampling
  int resArr[col];           // Stores each nCr result for reservoir sampling
  int resLower = 0;          // Lower bound for reservoir sampling
  int resUpper = col-1;      // Upper bound for reservoir sampling
  for(unsigned char i=0; i<col; i++){ // For each nCr result...
    resArr[i] = nCrArr[i];   // Store that result in the reservoir sampling array
  }
  for(unsigned char i=0; i<col; i++){                                      // For each nCr result...
    int randCombo = random(resLower,resUpper+1);                  // Generate a unique position to grab a number from
    nCrArr[i] = resSort(resArr, randCombo, &resUpper, &resLower); // Perform a reservoir sample to store and eliminate the number.
  }
  // Transpose the array to get our numbers....
  for(unsigned char i=0; i<8; i++){                                  // For each column array unsigned char in the chunk...
    for(unsigned char j=0; j<8; j++){                                // For each row array unsigned char in the chunk...
      if(j<row && i<col){                                   // If a pattern was generated for this row and column...
        BIT_WRITE(chunkPattern[j], i, BIT_READ(nCrArr[i], j)); // Store the jth bool of the ith nCr array in the ith bool of the jth solution
      }
      else{                                                 // Otherwise, theres no relevant data to store...
        BIT_CLEAR(chunkPattern[j], i);                       // Store a placeholder 0 in the ith bool of the jth solution
      }
    }
  }
  printf("Generated chunk of size %d x %d\n",row,col);
  int transCombos;                                                 // Stores the total combinations for all valid combinations of ones of the transposed chunk
  for(unsigned char i=0; i<numOnes; i++){                                   // For each possible combination of ones...
    if(genOdds){                                                   // If we're generating odd arrays..
      transCombos += nCr(ODD_NUM(i+1),col);                      // (2*i)+1 Calculate the number of odd combinations possible for all ones within the number of columns
    } else{                                                        // Otherwise...
      transCombos += nCr(EVEN_NUM(i+1),col);                         // 2*(i+1) Calculate the number of even combinations possible for all ones within the number of columns
    }
  }

  // Determine which dummy switches cannot be allowed to generate
  const unsigned char wrongCombos = (1 << col) - 1;        // (2^col - 1) Determine the maximum combinations of ones for the column size
  bool dummyCounter[wrongCombos];                   // Stores a flag indicating every wrong array unsigned char
  for(unsigned char i=0; i<wrongCombos; i++){
    dummyCounter[i] = 0;
  }
  unsigned char dummyCounterr = 0;
  dummyCounter[wrongCombos] = 1;                  // (2^col - 1) is never a valid array unsigned char
  dummyCounter[0] = 1;                            // 0 is never a valid array unsigned char
  for(unsigned char i=1; i<row; i++){                      // For each number of array bytes in the solution to XOR, aside from all of them...
    unsigned char wrongCombos = nCr(i, row);               // Determine the number of times each array unsigned char can be combined uniquely
    for(unsigned char j=0; j<wrongCombos; j++){            // For each unique array unsigned char combo of i array bytes...
      unsigned char wrongArray = wrongCombos;              // Initialize wrong array unsigned char
      unsigned char arraysToXOR = nCrScrambler(i, row, j); // Determine the array bytes to XOR
      for(unsigned char k=0; k<row; k++){                  // For each array unsigned char in the chunk...
        if(BIT_READ(arraysToXOR, k)){               // If we need to XOR this array unsigned char...
          wrongArray ^= chunkPattern[k];          // XOR this array unsigned char to produce a dummy that would ruin a unique solution
        }
      }
      dummyCounter[wrongArray] = 1;                   // Flag the generated array unsigned char as a dummy to avoid
    }
  }
  //Serial.println("Dummies to use in this chunk...");
  for(unsigned char i=0; i<(1 << col) - 1; i++){
    if(!dummyCounter[i]){
      dummyCounterr++;
      //Serial.println(i);
    }
  }
  printf("Found %d valid dummy arrays.\n",dummyCounterr);
  //if(!isSolutionValid(chunkPattern, row) && transCombos >= col){ // If the solution has duplicates and it's possible to make them unique...
  //    return 1;                                                    // Bad solution, flag for a redo
  //}
  /*if(false){ // We ONLY need to perform this during odd generation (correct arrays).
    // Determine which dummy switches cannot be allowed to generate
    unsigned char dummyCombos = (1 << col) - 1; // Total possible arrays
    bool resCount[dummyCombos];            // Stores every occurence of an array during reservoir sampling
    int resDummy[dummyCombos];            // Stores all possible unique dummy arrays that will not produce a solution
    resLower = 1;                          // Lower bound for reservoir sampling. Prefilter all 0's
    resUpper = dummyCombos-2;              // Upper bound for reservoir sampling. Prefilter all 1's
    for(unsigned char i=0; i<dummyCombos; i++){ // For each possible array...
      resCount[i] = 0;                 // Initialize array count occurences to 0. Necessary for repeat calls to function.
      resDummy[i] = i;                 // Store that array for reservoir sampling
    }

    for(unsigned char i=0; i<row; i++){                      // For each correct array...
      solution[i] = i;                                     // Initialize the positions of the solution arrays
      resSort(resDummy, chunkPattern[i], &resUpper, &resLower); // Perform a reservoir sample to eliminate the number.
      resCount[chunkPattern[i]] = 1;                            // Flag this number for being removed as a dummy option
    }
    for(unsigned char i=1; i<row; i++){                      // For each number of arrays in the solution to XOR, aside from all of them...
      int wrongCombos = nCr(i, row);              // Determine the number of times each array can be combined uniquely
      for(int j=0; j<wrongCombos; j++){                    // For the number times we can combine each array...
        int wrongArray = 0;                               // Stores resulting wrong array
        unsigned char arraysToXOR = nCrScrambler(i, row, j); // Determine the array combo to XOR
        for(unsigned char k=0; k<row; k++){                  // For each array in the solution...
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
    for(unsigned char i=row; i<col; i++){                          // For the number of dummy arrays to produce...
      int randArr = random(resLower,resUpper+1);                          // Generate a random position to pull a dummy array from the dummy reservoir
      chunkPattern[i] = resSort(resDummy, randArr, &resUpper, &resLower); // Perform a reservoir sample to generate a unique, valid dummy array
    }
  }*/

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
  printf("Generating a boolean array of %d row bytes and %d column bytes...\n",rowChunk,colChunk);
  //Determine the number of chunks to produce and their location
  int correctToGo = numSolution; // Store the number of correct arrays to generate to count
  int correctRow = DIVIDE_POW2D(numSolution,3) + (numSolution % 8 > 0); // Store the number of row chunks to generate to complete the correct arrays
  unsigned char n[8] = {0,0,0,0,0,0,0,0};
  for(int i=0; i<numSolution; i++){ // For each solution array...
    solution[i] = i; // Initialize the index of each solution array
  }
  for(unsigned char i=0; i<correctRow; i++){ // For each row chunk to create (vertical)...
    unsigned char rows = 8;
    if(!(DIVIDE_POW2D(correctToGo,3))){ // If we have less than 8 solutions to generate...
      rows = correctToGo; // Make the odds generated equal to the last bit of solution arrays
    }
    if(i == correctRow - 1 && lastRow){ //If this is the last row chunk to create and we have an incomplete chunk to make...
      rows = lastRow; // Resize the row dimension of the chunk to the number of arrays in the last row
    }
    for(unsigned char j=0; j<colChunk; j++){ // For each column chunk to create (horizontal)...
      unsigned char columns = 8;
      if(j == colChunk - 1 && lastCol){ // If this is the last column chunk to create and we have an incomplete chunk to make...
        columns = lastCol;              // Resize the column dimension of the chunk to the amount of bools in the last column
      }
      bool flag_redo; // Flag to trigger a regeneration
      //Serial.print("Generating chunk at position [");;
      //Serial.print(i);
      //Serial.print(",");
      //Serial.print(j);
      //Serial.println("]");
      flag_redo = generateChunk(rows, columns, (i==0));
      while(flag_redo){
        printf("I made a bad chunk. Let's try again...\n");
        flag_redo = generateChunk(rows, columns, (i==0));
      }
      for(unsigned char k=0; k<8; k++){
        pattern[i][j][k] = chunkPattern[k];
        for(unsigned char m=0; m<8; m++){
        }
      }
    }
    correctToGo -= rows; // Check off the number of correct array generated
  }
}
  /*// FINALLY, we can produce proper dummy arrays
  for(unsigned char i=0; i<numRow - numSolution; i++){                       // For the number of dummy arrays to produce...
    int patternUpper = i + numSolution;                            // Upper bound of the current pattern array
    int randArr = random(resLower,resUpper+1);                    // Generate a random position to pull a dummy array from the dummy reservoir
    int randPos = random(0,patternUpper+1);                       // Generate a random position in the pattern to place the dummy array
    int dummy = resSort(resDummy, randArr, &resUpper, &resLower); // Perform a reservoir sample to generate a unique, valid dummy array
    shiftInRight(dummy, randPos, patternUpper);                    // Insert the dummy into the pattern
    // Update the position of the solution arrays
    for(unsigned char j=0; j<numSolution; j++){  // For each solution array...
      if(solution[j] >= randPos){      // If the inserted dummy array shifted the current solution array over...
        solution[j]++;                 // Update the position of the solution array
      }
    }
  }
  return 0;
}*/
void printRowDivider(bool index = 0){
  printf("   |");                      // Print a row chunk divider
  for(unsigned char i=0; i<colChunk; i++){      // For each column chunk...
    unsigned char colsToPrint = 8;              // Default to 8 column unsigned char dividers to print
    if(i == colChunk - 1 && lastCol){  // But if it's the last column and it isn't full...
      colsToPrint = lastCol;           // Print dividers for only the remaining column bytes
    }
    for(unsigned char j=0; j<colsToPrint; j++){
      if(index){
        if(j + MULTIPLY_POW2(i,3) < 10){         // Print the index of the array, with formatting up to 999
          printf("0");
        }
        if(j + MULTIPLY_POW2(i,3) < 100){
          printf("0");
        }
        printf("%d",j + MULTIPLY_POW2(i,3));
      } else{
        printf("---");
      }
      printf("-");
    }
    printf(" | ");
  }
  printf("\n");
}

void printPuzzle(){
  printf("The arrays generated, top to bottom, are...\n");
  printRowDivider();  // Divider
  printRowDivider(1); // Index line
  printRowDivider();  // Divider
  for(unsigned char i=0; i<rowChunk; i++){                    // For each chunk of rows...
    unsigned char rowsToPrint = 8;                            // Default to 8 row unsigned char arrays to print
    if(i == rowChunk - 1 && lastRow){                // But if it's the last row and it isn't full...
      rowsToPrint = lastRow;                         // Print only the remaining rows
    }
    for(unsigned char j=0; j<rowsToPrint; j++){               // For each row unsigned char array in the chunk to print...
      if(j + MULTIPLY_POW2(i,3) < 10){         // Print the index of the array, with formatting up to 999
        printf("0");
      }
      if(j + MULTIPLY_POW2(i,3) < 100){
        printf("0");
      }
      printf("%d",j + MULTIPLY_POW2(i,3));
      printf("|");
      for(unsigned char k=0; k<colChunk; k++){                // For each chunk of columns...
        unsigned char colsToPrint = 8;                        // Default to 8 column unsigned char arrays to print
        if(k == colChunk - 1 && lastCol){            // But if it's the last column and it isn't full...
          colsToPrint = lastCol;                     // Print only the remaining columns
        }
        for(unsigned char m=0; m<colsToPrint; m++){           // For each column unsigned char array in the chunk to print...
          printf(" %d  ",BIT_READ(pattern[i][k][j],m)); // Actual print of the bool value
        }
        printf(" | ");
      }
      printf("\n");
    }
    printRowDivider();  // Divider
    printRowDivider(1); // Index line
    printRowDivider();  // Divider
  }
}

void printChunk(unsigned char chunk[8]){
  printf("The chunk generated, top to bottom, is...\n");
  printf("  ,");
  for(unsigned char i=0; i<8; i++){
    printf("___");
  }
  printf("\n");
  for(unsigned char i=0; i<8; i++){
    if(i < 10){
      printf("0");
    }
    printf("%d|",i);
    for(unsigned char j=0; j<8; j++){
      printf("  %d ",BIT_READ(chunk[i],j));
    }
    printf("\n");
  }
}

int main() {
  srand(time(NULL));          // Use noise fluxuations from the A0 pin to seed the RNG
  generateSolution();          // Create a valid solution for our predefined array size
  // Print our solution
  printPuzzle();
  printf("The solution arrays are at indexes: ");
  for(int i=0; i<numSolution; i++){ // For each solution array..
    printf("%d ",solution[i]);
  }
  printf("\nFinding valid dummies...\n");
  findDummies(0);
  printf("\nFound valid dummies at...\n");
  unsigned char dummyCounter = 0;
  for(int i=0; i<255; i++){ // For each array unsigned char combo in col...
    if(!dummyCount[0][i]){
      printf("%d, ",i);
      dummyCounter++;
    }
  }
  printf("\nThere are %d valid dummies", dummyCounter);
}
