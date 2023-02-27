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
#include <stdbool.h>
#include <time.h>

 /*
  * Preprocessor definitions to easily implement Arduino sketches as regular C files.
  * Avoid using Serial.print and use printf when possible. Comment out this preprocessor
  * section when uploading to Arduino.
  */
#define random(min,max) ((rand()%(max-min))+min)        // Arduino implementation of RNG where a range between min and max is generated, but max is exclusive
#define bitRead(a,b) (!!((a) & (1ULL<<(b))))            // Arduino bitRead(a,b), where a = target byte, b = bit position
#define bitWrite(a,b,x) ((a) = (a & ~(1ULL<<b))|(x<<b)) // Arduino bitWrite(a,b,x), where a = target byte, b = bit position, x = value
typedef unsigned char byte;                             // Arduino declaration of an unsigned char explicitly used as a number

#define DEBUG false
#define numSolution 10 // Number of arrays in the unique solution
#define numRow      30 // Total number of arrays
#define arrSize     30 // Total number of boolean vals in an array

#define lastRow  (numRow%8)                    // Amount of arrays to create in the last row chunk
#define lastCol  (arrSize%8)                   // Amount of bools to create in the last column chunk
#define rowChunk ((numRow>>3ULL)+(lastRow>0))  // (numRow/8) Determine how many rows of chunks to make
#define colChunk ((arrSize>>3ULL)+(lastCol>0)) // (arrSize/8) Determine how many columns of chunks to create
byte pattern[rowChunk][colChunk][8];           // Stores: Valid pattern
int solution[numSolution];                     //       : Bool index location of the solution arrays
byte chunkPattern[12];                         //       : Bool chunk for each 8x8(normal) to 12x12(big) chunk generation
bool dummyCount[colChunk][256];                //       : Valid and invalid dummy arrays that can be generated

#define bitSet(a,b) ((a) |= (1ULL<<(b)))    // Set bit in position b to 1 in target byte a
#define bitClear(a,b) ((a) &= ~(1ULL<<(b))) // Set bit in position b to 0 in target byte a
#define bitFlip(a,b) ((a) ^= (1ULL<<(b)))   // Set bit in position b to ~b in target byte a
#define ODD_NUM(n) (((n-1)<<1ULL)|1ULL)     // The nth odd number (1,3,5,etc...)
#define EVEN_NUM(n) ((i+1)<<1ULL)           // The nth even num (2,4,6,etc...)
#define NUM_ODD(n) ((n&1ULL)+(n>>1))        // The number of odd numbers from 1 to n
#define NUM_EVEN(n) ((n<<1ULL))             // The number of even numbers from 0 to n
#define MULTIPLY_POW2(n,m) (n<<m)           // Multiply n by 2^m
#define DIVIDE_POW2D(n,m) (n>>m)            // Divide n by 2^m, rounded down
#define DIVIDE_POW2U(n,m) ((n>>m)+(n&1ULL)) // Divide n by 2^m, rounded up

// Function prototypes
void shiftOutLeft(int arr[], int* upper, int pos);
void shiftOutRight(int arr[], int* lower, int pos);
void shiftInRight(int insert, int pos, int upper);
int resSort(int arr[], int pos, int* upper, int* lower);
bool dummyChecker(unsigned long long val, int col, unsigned long long solutionArr[], unsigned long long maxNum);
void generateBigChunk(int row, int col, bool genOdds);
void printRowDivider(bool index);
void printPuzzle();
void printChunk(byte chunk[12]);

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
void shiftOutLeft(int arr[], int* upper, int pos) {
    for (int i = pos; i < *upper; i++) { // Starting at index=pos, for each index of the array to the upper bound...
        arr[i] = arr[i + 1];      // Shift index on the right to the current index.
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
void shiftOutRight(int arr[], int* lower, int pos) {
    for (int i = pos - 1; i >= *lower; i--) { // Starting at index=pos-1, for each index of the array to resLower...
        arr[i + 1] = arr[i];         // Shift index on the current index to the right.
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
int resSort(int arr[], int pos, int* upper, int* lower) {
    int num = arr[pos];                       // Store that number
    if (arr[pos - (*lower)] < arr[(*upper) - pos]) // If that number is closer to the left side of the reservoir sampling array...
        shiftOutRight(arr, lower, pos);         // Shift reservoir sampling array right
    else
        shiftOutLeft(arr, upper, pos);          // Otherwise, shift reservoir sampling array left
    return num;                               // Return the number shifted out of the reservoir sampling array
}

unsigned long long bigRand(int col) {
    unsigned long long randomNum = rand() %  2ULL;
    for (int i = 0; i < col-1; i++) {
        randomNum <<= 1ULL;
        randomNum += rand() % 2ULL;
    }

    return randomNum;
}

/*
* Function: dummyChecker
* Checks a single row against an inputted array for uniqueness (not a duplicate of another row) and 
* exclusiveness (cannot be involved in a solution). Returns true if conditions are met.
*/
bool dummyChecker(unsigned long long val, int col, unsigned long long solutionArr[], unsigned long long maxNum) {

    unsigned long long maxVal = MULTIPLY_POW2(1, col) - 1; // Max value for column number of bits
    // Determine which dummy switches cannot be allowed to generate
    printf("Checking %d for validity...", val);
    for (unsigned long long i = 1; i < maxVal + 1; i++) { // For every possible combination of solution vals to compare (0-255)...
        unsigned long long valXOR = val; // Store the inputted val to begin XOR
        for (int j = 0; j < col; j++) { // For each val in the solution array...(0-7)
            if (bitRead(i, j)) { // If we need to XOR the solution val...(arr[0]-arr[7])
                if (solutionArr[j] == val) {
                    printf("NO\n");
                    return 0;
                }
                valXOR ^= solutionArr[j]; // XOR the inputted val with the solution val
                if (valXOR == maxNum) { // If we found a failing combination...
                    printf("NO\n");
                    return 0; // Immediately exit
                }
            }
        }
    }
    printf("OK\n");
    return 1;
}

/*
* Function: generateBigChunk()
* 
* Accepts a row dimension, column dimension, and a boolean to determine whether to generate an
* even or odd array. All rows will be unique and the numSolution rows that uniquely combine to
* a sum of 2^columns - 1 or 0, depending on if even or odd is chosen.
*/
void generateBigChunk(int row, int col, bool genOdds) {
    unsigned long long* solutionArr = (unsigned long long*)calloc(col, sizeof(unsigned long long));
    unsigned long long maxVal = MULTIPLY_POW2(1,col) - 1; // Max value for column number of bits
    solutionArr[0] = maxVal;
    unsigned long long randVal;
    bool flag_unique = 0;  // Flag to raise if RNG value is found to be unique
    // Find correct vals
    for (int i = 0; i < numSolution - 1; i++) {  // For each solution we have to generate...
        while (!flag_unique) {                   // While we still don't have a unique number...
            flag_unique = 1;
            randVal = bigRand(col);         // Generate a random number in range 1 to maxVal-1
            for (int j = 0; j <= i; j++) {       // For each solution we've already generated...
                if (randVal == solutionArr[j]) { // If we find it already exists in our solution...
                    flag_unique = 0;             // Flag the random number for a redo
                }
            }                                    
        }                                        // Otherwise, we escape with our unique number
        flag_unique = 0;
        solutionArr[i] ^= randVal;               // Overwrite the last XOR'd number with the XOR of the unique RNG number
        solutionArr[i + 1] = randVal;            // Store the unique RNG number as a solution
    }

    // Find dummies
    for (int i = 0; i < row - numSolution; i++) {  // For each dummy we have to generate...
        while (!flag_unique) {                   // While we still don't have a valid dummy...
            randVal = bigRand(col);         // Generate a random number in range 1 to maxVal-1
            flag_unique = dummyChecker(randVal, numSolution+i, solutionArr, maxVal); // Check the validity of the dummy
        }                                        // Otherwise, we escape with our unique number
        flag_unique = 0;
        solutionArr[numSolution+i] = randVal;            // Store the unique RNG number as a solution
    }

    // Print out the bits
    for (int i = 0; i < row; i++) {
        for (int j = 0; j < col; j++) {
            printf("%d ", bitRead(solutionArr[i], j));
        }
        printf("\n");
    }
    
    //printChunk(chunkPattern);
}

void printRowDivider(bool index) {
    printf("   |");                      // Print a row chunk divider
    for (byte i = 0; i < colChunk; i++) {      // For each column chunk...
        byte colsToPrint = 8;              // Default to 8 column byte dividers to print
        if (i == colChunk - 1 && lastCol) {  // But if it's the last column and it isn't full...
            colsToPrint = lastCol;           // Print dividers for only the remaining column bytes
        }
        for (byte j = 0; j < colsToPrint; j++) {
            if (index) {
                if (j + MULTIPLY_POW2(i, 3) < 10) {         // Print the index of the array, with formatting up to 999
                    printf("0");
                }
                if (j + MULTIPLY_POW2(i, 3) < 100) {
                    printf("0");
                }
                printf("%d", j + MULTIPLY_POW2(i, 3));
            }
            else {
                printf("---");
            }
            printf("-");
        }
        printf(" | ");
    }
    printf("\n");
}

void printPuzzle() {
    printf("The arrays generated, top to bottom, are...\n");
    printRowDivider(0); // Divider
    printRowDivider(1); // Index line
    printRowDivider(0); // Divider
    for (byte i = 0; i < rowChunk; i++) {                      // For each chunk of rows...
        byte rowsToPrint = 8;                              // Default to 8 row byte arrays to print
        if (i == rowChunk - 1 && lastRow) {                  // But if it's the last row and it isn't full...
            rowsToPrint = lastRow;                           // Print only the remaining rows
        }
        for (byte j = 0; j < rowsToPrint; j++) {                 // For each row byte array in the chunk to print...
            if (j + MULTIPLY_POW2(i, 3) < 10) {                 // Print the index of the array, with formatting up to 999
                printf("0");
            }
            if (j + MULTIPLY_POW2(i, 3) < 100) {
                printf("0");
            }
            printf("%d", j + MULTIPLY_POW2(i, 3));
            printf("|");
            for (byte k = 0; k < colChunk; k++) {                   // For each chunk of columns...
                byte colsToPrint = 8;                           // Default to 8 column byte arrays to print
                if (k == colChunk - 1 && lastCol) {               // But if it's the last column and it isn't full...
                    colsToPrint = lastCol;                        // Print only the remaining columns
                }
                for (byte m = 0; m < colsToPrint; m++) {              // For each column byte array in the chunk to print...
                    printf(" %d  ", bitRead(pattern[i][k][j], m)); // Actual print of the bool value
                }
                printf(" | ");
            }
            printf("\n");
        }
        printRowDivider(0); // Divider
        printRowDivider(1); // Index line
        printRowDivider(0); // Divider
    }
}

void printChunk(byte chunk[12]) {
    printf("The chunk generated, top to bottom, is...\n");
    printf("  ,");
    for (byte i = 0; i < 8; i++) {
        printf("___");
    }
    printf("\n");
    for (byte i = 0; i < 8; i++) {
        if (i < 10) {
            printf("0");
        }
        printf("%d|", i);
        for (byte j = 0; j < 8; j++) {
            printf("  %d ", bitRead(chunk[i], j));
        }
        printf("\n");
    }
}

int main() {
    
    srand(time(NULL));          // Use noise fluxuations from the A0 pin to seed the RNG
    generateBigChunk(numRow,arrSize,1);
    printf("The solution arrays are at indexes: ");
    for (int i = 0; i < numSolution; i++) { // For each solution array..
        printf("%d ", i);
    }
}
