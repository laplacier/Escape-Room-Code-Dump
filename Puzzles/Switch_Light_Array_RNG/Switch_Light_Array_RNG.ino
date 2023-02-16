/*
 * ESCAPE ROOM SWITCH LIGHT ARRAY DEMO
 * 
 * You have a large array of switches and lights. Players must
 * flip the switches, but only a certain combination of 
 * flipped switches will trigger the solution and allow
 * the players to progress through the game!
 * 
 * This implementation allows for seamless detection of an endless
 * amount of wires connected correctly with very few IO pins.
 * Tired of building labor intensive resistor or keypad arrays,
 * having them degrade over time, and reading noisy analog values?
 * Me too!
 * 
 * This implementation requires 5 IO pins to control any amount of 
 * PISO and SIPO registers.
 * 
 * by Adam Billingsley
 * created  6 Feb, 2023
 * modified 7 Feb, 2023
 */

//-------------- SETTINGS & GLOBAL VARIABLES -----------------//
#define DEBUG true           // Prints debug information to serial if true
#define numSwitch   8        // Total switches in the puzzle
#define numSolution 5        // Total switches involved in puzzle solution
#define numLight    8        // Total lights in the puzzle
#define numPISO     1        // Number of PISO registers in daisy chain
#define numSIPO     1        // Number of SIPO registers in daisy chain
byte inputData[numPISO];     // Stores PISO data
byte outputData[numSIPO];    // Stores data to send to SIPO registers
byte inputOld[numPISO];      // Stores previous PISO data
byte outputOld[numSIPO];     // Stores previous data sent to SIPO registers
bool flag_solved = false;    // Flag for completed puzzle

//------------------ PIN DEFINITIONS -------------------------//
#define clockPin   7 // Pin 2 on all SN74HC165N and Pin 11 on all SN74HC595N
#define dataInPin  3 // Pin 7 on FIRST SN74HC165
#define loadPin    2 // Pin 1 on all SN74HC165
#define latchPin   4 // Pin 12 on all SN74HC595N
#define dataOutPin 6 // Pin 14 on FIRST SN74HC595N

//---------------- FUNCTION PROTOTYPES -----------------------//
bool checkWires();
bool isDataNew();
void printData(byte data, String regName, int regNum);
void pulsePin(int pinName, int pulseTime);
void readPISO(byte data[numPISO]);
void sendSIPO(byte data[numSIPO]);
void debug();

void setup(){
  if(DEBUG)
    Serial.begin(19200);

  // Seed RNG for solution
  randomSeed(analogRead(A0));
  
  // Set all the pins of SN74HC165N
  pinMode (loadPin, OUTPUT);
  pinMode (dataInPin, INPUT);

  // Set all the pins of SN74HC595N
  pinMode(latchPin, OUTPUT);
  pinMode(dataOutPin, OUTPUT);

  // The clock pin is shared between ALL registers
  pinMode (clockPin, OUTPUT);
}

void loop(){
  if(true){ //If the switches are all toggled correctly...
    //The puzzle is solved, do a thing!
  }
  if(DEBUG)
    debug();
}
