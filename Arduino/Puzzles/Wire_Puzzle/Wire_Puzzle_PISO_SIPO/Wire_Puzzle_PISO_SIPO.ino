/*
 * ESCAPE ROOM WIRE PUZZLE DEMO
 * 
 * You have a large array of plugs. Players have wires they must
 * connect to the plugs, but only a certain combination of 
 * connections between plugs will trigger the solution and allow
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

//-------------- SETTINGS & GLOBAL CONSTANTS -----------------//
#define DEBUG true       // Prints debug information to serial if true
#define numWires   8     // Total wires in the puzzle solution
#define numPISO    1     // Number of PISO registers in daisy chain
#define numSIPO    1     // Number of SIPO registers in daisy chain
byte inputData[numPISO]; // Stores PISO data


//------------------ PUZZLE SOLUTION -------------------------//
const int solution[numWires][2] = {
  {0,0}, //PISO#0 pin A, SIPO#0 pin A
  {1,1}, //PISO#0 pin B, SIPO#0 pin B
  {2,2}, //PISO#0 pin C, SIPO#0 pin C
  {3,3}, //PISO#0 pin D, SIPO#0 pin D
  {4,4}, //PISO#0 pin E, SIPO#0 pin E
  {5,5}, //PISO#0 pin F, SIPO#0 pin F
  {6,6}, //PISO#0 pin G, SIPO#0 pin G
  {7,7}, //PISO#0 pin H, SIPO#0 pin H
};

//------------------ PIN DEFINITIONS -------------------------//
#define clockPin   7 // Pin 2 on all SN74HC165N and Pin 11 on all SN74HC595N
#define dataInPin  3 // Pin 7 on FIRST SN74HC165
#define loadPin    2 // Pin 1 on all SN74HC165
#define latchPin   4 // Pin 12 on all SN74HC595N
#define dataOutPin 6 // Pin 14 on FIRST SN74HC595N

//---------------- FUNCTION PROTOTYPES -----------------------//
bool checkWires();
void printData(byte data, String regName, int regNum);
void pulsePin(int pinName, int pulseTime);
void readPISO(byte data[numPISO]);
void sendSIPO(byte data[numSIPO]);

void setup(){
  if(DEBUG)
    Serial.begin(19200);

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
  if(checkWires()){ //If the wires are all connected properly...
    //The puzzle is solved!
    if(DEBUG)
      Serial.println("All wires connected, puzzle solved!");
  }
  delay(500);
}
