/*
 * ESCAPE ROOM WIRE PUZZLE DEMO
 * 
 * You have a large array of plugs. Players have wires they must
 * connect to the plugs, but only a certain combination of 
 * connections between plugs will trigger the solution and allow
 * the players to progress through the game!
 * 
 * This implementation allows for seamless detection of wires
 * connected correctly with minimal external components.
 * Tired of building labor intensive resistor or keypad arrays,
 * having them degrade over time, and reading noisy analog values?
 * Me too!
 * 
 * This implementation requires 1 IO pin assigment for each
 * wire connection and 3 IO pins to control any amount of PISO
 * registers.
 * 
 * by Adam Billingsley
 * created  6 Feb, 2023
 * modified 7 Feb, 2023
 */

//-------------- SETTINGS & GLOBAL CONSTANTS -----------------//
#define DEBUG true       // Prints debug information to serial if true
#define numWires  8      // Total wires in the puzzle solution
#define numPISO   1      // Number of PISO registers in daisy chain
byte inputData[numPISO]; // Stores PISO data


#define clockPin  9      // Pin 2 of SN74HC165
#define dataInPin 10     // Pin 7 of SN74HC165
#define loadPin   11     // Pin 1 of SN74HC165

//------------------ PUZZLE SOLUTION -------------------------//
const int solution[numWires][2] = {
  {1,0}, // DIO Pin 1, PISO pin A
  {2,1}, // DIO Pin 2, PISO pin B
  {3,2}, // DIO Pin 3, PISO pin C
  {4,3}, // DIO Pin 4, PISO pin D
  {5,4}, // DIO Pin 5, PISO pin E
  {6,5}, // DIO Pin 6, PISO pin F
  {7,6}, // DIO Pin 7, PISO pin G
  {8,7}, // DIO Pin 8, PISO pin H
};

//---------------- FUNCTION PROTOTYPES -----------------------//
bool checkWires();
void pulsePin(int pinName, int pulseTime);
void readPISO(byte data[numPISO]);
bool checkWires();

void setup(){
  if(DEBUG)
    Serial.begin(19200);
  
  //All wire plugs initialized as inputs
  for(int i=0; i<numWires; i++){
    pinMode(solution[i][0], INPUT_PULLUP);
  }

  //PISO register pins
  pinMode(loadPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataInPin, INPUT);
}

void loop(){
  if(checkWires()){ //If the wires are all connected properly...
    //The puzzle is solved!
    if(DEBUG)
      Serial.println("All wires connected, puzzle solved!");
  }
  delay(500);
}
