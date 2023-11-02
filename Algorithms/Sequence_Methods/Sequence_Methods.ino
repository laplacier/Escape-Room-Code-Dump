#define NUM_SEQUENCE 12

const uint8_t solution_sequence[NUM_SEQUENCE] = {0,1,2,3,4,5,6,7,8,9,10,11};
uint8_t input_sequence[NUM_SEQUENCE] = {16,16,16,16,16,16,16,16,16,16,16,16};
uint8_t sequenceCounter = 0;

void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

}

/*
 * Keeps a running history of insertions and queues
 * when the correct sequence is hit
 * 
 * Pros 
 * -----------
 *  - Have a history of taps at all times
 *  - No taps are lost
 *  
 * Cons
 * -----------
 *  - Point at which the solve attempt starts and stops is vague
 */
void insertSequenceRolling(uint8_t input){
  bool isSequenceSolved = true; 
  for(int i=0; i<NUM_SEQUENCE - 1; i++){
    input_sequence[i] = input_sequence[i+1];
  }
  input_sequence[NUM_SEQUENCE - 1] = input;

  for(int i=0; i<NUM_SEQUENCE; i++){
    if(input_sequence[i] != solution_sequence[i]){
      isSequenceSolved = false;
    }
    Serial.print(input_sequence[i]);
    Serial.print(" ");
  }
  Serial.println();
  if(isSequenceSolved){
    Serial.println("Solved!");
  }
}

/*
 * Keeps a count of correct inputs in sequence
 * and resets if incorrect
 * 
 * Pros 
 * -----------
 *  - Lightweight
 *  - Clear reset of sequence indicated by counter
 *  
 * Cons
 * -----------
 *  - No history of inputs
 */
void insertSequenceStarting(uint8_t input){
  if(input != sequence_solution[sequenceCounter]){
    sequenceCounter = 0;
    // play wrong led and wrong sound
  }
  else{
    setTamBlue(sequenceCounter);
    sequenceCounter++;
  }

  if(sequenceCounter >= NUM_SEQUENCE){
    // play winning led and win sound
  }
}
