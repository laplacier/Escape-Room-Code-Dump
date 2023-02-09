/*
 * ROTARY ENCODER DEMO
 * 
 * Suppose you want to rotate a prop in your room. With that prop, 
 * you want to detect how many turns the prop makes or which
 * direction the prop is rotated. Of the many innovative solutions,
 * a rotary encoder can do the job!
 * 
 * Use rotary encoders for:
 * -The angle of rotation of an attached prop
 * -The direction of rotation of an attached prop
 * -The angular distance of an attached prop (# of rotations)
 * 
 * Do NOT use rotary encoders:
 * -To support the weight of your prop.
 * -In props where the rotary implement of the encoder will
 *   experience unintended lateral or vertical forces.
 * -With programs with delay()s or other blocking code.
 * -In high speed scenarios where high frequency polling and 
 *   interrupts are required. This exceeds the scope of this 
 *   example! I recommend the Encoder.h library by Paul 
 *   Stoffregen for those purposes.
 * 
 * This implementation requires 2 IO pins per rotary encoder.
 * 
 * by Adam Billingsley
 * created  7 Feb, 2023
 * modified 8 Feb, 2023
 */


/* 
 * An encoder, at minimum, has two IO pins, A and B, and a 
 * GND pin. Pins A and B must be pulled high through software
 * with INPUT_PULLUP or externally with resistors. For encoders 
 * with builtin pull-up resistors, they will also include a VCC 
 * pin. Each time you turn a rotary encoder, each pin follows 
 * the same, slightly out of sync electrical pattern.
 *      
 *      Turning clockwise ------------------------>
 *                       <------------------------ Turning counterclockwise
 *   digitalRead(Pin A)  1 0   0 1   1 0   0 1   1
 *   digitalRead(Pin B)  0 0   1 1   0 0   1 1   0...
 *                       v v   v v   v v   v v   v
 *                        _____       _____       __
 *               Pin A __|     |_____|     |_____|    
 *                          _____       _____       
 *               Pin B ____|     |_____|     |______
 *                       ^ ^   ^ ^   ^ ^   ^ ^   ^
 * As long as we are polling each pin fast enough not to miss
 * any pin changes, we can accurately determine a rotation in
 * a specific direction. We can see that if Pin A starts at 1
 * and Pin B starts at 0, then Pin A stays 1 while Pin B becomes
 * 1, we can determine the encoder was rotated clockwise.
 * 
 * Using this behavior, we can create a boolean logic table by
 * mapping each possible combination of new and old pin values
 * to the resulting direction change detected.
 *                  
 *                | newPin||oldPin|(-CCW/+CW)
 *           byte | B   A   B   A | DIR
 *              0 | 0   0   0   0 |  0
 *              1 | 0   0   0   1 | -1
 *              2 | 0   0   1   0 | +1
 *              3 | 0   0   1   1 |+-2
 *              4 | 0   1   0   0 | +1
 *              5 | 0   1   0   1 |  0
 *              6 | 0   1   1   0 |+-2
 *              7 | 0   1   1   1 | -1
 *              8 | 1   0   0   0 | -1
 *              9 | 1   0   0   1 |+-2
 *             10 | 1   0   1   0 |  0
 *             11 | 1   0   1   1 | +1
 *             12 | 1   1   0   0 |+-2
 *             13 | 1   1   0   1 | +1
 *             14 | 1   1   1   0 | -1
 *             15 | 1   1   1   1 |  0
 * 
 * Changes of +-2 occur when encoder polling has missed a pulse and
 * is now reading the next one. This can occur when polling is not
 * called fast enough to handle the speed the encoder is rotating.
 * Interrupts can detect a rising or falling edge of a pin to
 * accurately determine the direction in these scenarios, but edges
 * cannot be detected when polling. We can, however, make an educated
 * prediction based on the direction the last rotation occurred.
 * Polling occurs faster than a human can react, so it is *mostly*
 * safe to assume that if a skip is detected and the previous poll 
 * was counterclockwise, the skip detected also occurred in the
 * counterclockwise direction.
 * 
 * A change in state of an encoder is referred to as a pulse. By 
 * counting the number of detected pulses when rotating the encoder 
 * one full rotation, we can determine the resolution of the encoder, 
 * and the angle of rotation per pulse. We can also add or subtract 
 * from the total pulses depending on which direction the rotation 
 * occurs to find the angular distance.
 */

#define DEBUG true            // Toggle printing to serial. Serial communication WILL significantly slow down polling.
#define encPinA 2             // Encoder pin A
#define encPinB 3             // Encoder pin B
#define pulsesPerRotation 400 // The resolution, or number of pulses required to make a full rotation. Varies between encoder models.
byte prevPin;                 // The last recorded state of encoder pin A and pin B
byte prevDir = 0;             // The last recorded direction change of the encoder
int encPosition = 0;          // The position of the encoder in # of detected pulses
bool ledState = 0;            // Built-in LED state flag

/* 
 * Function: encPoll
 * Accepts the encoder A and encoder B pins, combines the 
 * pin states into a single byte, and returns the pin 
 * states. The byte is constructed as b000000BA.
 */
byte encPoll(int PinA, int PinB) {
  bool A = digitalRead(PinA);             // Get encoder pin A state
  bool B = digitalRead(PinB);             // Get encoder pin B state
  byte result = (byte)(B << 1) | (byte)A; // Combine the pin states into a byte
  return result;                          // Return the byte
}

/* 
 * Function: encAngle
 * Accepts the encoder position and calculates the angle 
 * of rotation between 0-360 degrees bad on the defined
 * encoder pulses per rotation. Returns the encoder angle.
 */
float encAngle(int encPos){
  float angle;                                     // Float to store angle with decimal places
  encPos %= pulsesPerRotation;                     // Remove wraparound pulses to determine single rotation angle
  angle = (float)encPos / pulsesPerRotation * 360; // Calculate the angle from 0-360 degrees
  return angle;                                    // Return the angle
}

/* 
 * Function: encPrint
 * Accepts the encoder position and prints the resulting encoder 
 * angular distance and angle.
 */
void encPrint(int encPos){
  Serial.print("Distance: ");
  Serial.println(encPos);
  Serial.print("  Angle: ");
  Serial.println(encAngle(encPos),2);
}

void setup() {
  if(DEBUG){
    Serial.begin(19200);
    Serial.println();
  }

  //Encoder pins
  pinMode(encPinA, INPUT_PULLUP);
  pinMode(encPinB, INPUT_PULLUP);
  prevPin = encPoll(encPinA,encPinB); //Get the initial state of the encoder pins
  
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  byte newPin = encPoll(encPinA,encPinB); // Poll encoder, get pin states
  switch((newPin << 2) | prevPin){        // Check the boolean table for what rotation occurred...
    case 0: case 5: case 10: case 15:     // No rotation detected
      prevDir = 0;                        // Store no rotation detected for next poll
      break;
    case 2: case 4: case 11: case 13:     // Clockwise rotation detected
      prevDir = 1;                        // Store CW rotation detected for next poll
      encPosition++;                      // Add the CW rotation to the position counter
      led = !led;                         // Invert the state of the built-in LED
      digitalWrite(LED_BUILTIN, led);     // Write state to the built-in LED
      if(DEBUG)
          encPrint(encPosition);          // Print the updated states and calculations
      break;
    case 1: case 7: case 8: case 14:      // Counterclockwise rotation detected
      prevDir = -1;                       // Store CCW rotation detected for next poll
      encPosition--;                      // Subtract the CCW rotation from the position counter
      led = !led;                         // Invert the state of the built-in LED
      digitalWrite(LED_BUILTIN, led);     // Write state to the built-in LED
      if(DEBUG)
          encPrint(encPosition);          // Print the updated states and calculations
      break;
    case 3: case 6: case 9: case 12:      // A skip of a position detected. Two rotations in a direction
      if(prevDir > 0){                    // If the last rotation was clockwise...
        encPosition+=2;                   // Predict a double CW rotation
        if(DEBUG)
          encPrint(encPosition);          // Print the updated states and calculations
      }
      else if(prevDir < 0) {              // If the last rotation was counterclockwise...
        encPosition-=2;                   // Predict a double CCW rotation
        if(DEBUG)
          encPrint(encPosition);          // Print the updated states and calculations
      }
                                          // Otherwise, we can't predict the change, skip it
      break;
  }
  prevPin = newPin;                       // Store the pin state detected for next poll
}
