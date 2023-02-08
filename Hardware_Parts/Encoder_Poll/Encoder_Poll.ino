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
 */


/* 
 * An encoder, at minimum, has two IO pins, A and B, and a 
 * GND pin. Pins A and B must be pulled high through software
 * with INPUT_PULLUP or externally with resistors. Each time 
 * you turn a rotary encoder, each pin follows the same,
 * slightly out of sync electrical pattern.
 *      
 *      Turning clockwise ------------------------>
 *                       <------------------------ Turning counterclockwise
 *   digitalRead(Pin A)  0 1   1 0   0 1   1 0   0
 *   digitalRead(Pin B)  1 1   0 0   1 1   0 0   1...
 *                       v v   v v   v v   v v   v
 *                          _____       _____       
 *               Pin A ____|     |_____|     |______
 *                        _____       _____       __
 *               Pin B __|     |_____|     |_____|  
 *                       ^ ^   ^ ^   ^ ^   ^ ^   ^
 * As long as we are polling each pin fast enough not to miss
 * any pin changes, we can accurately determine a rotation in
 * a specific direction. We can see that if Pin A starts at 1
 * and Pin B starts at 0, then Pin A stays 1 while Pin B becomes
 * 1, we can determine the encoder was rotated counterclockwise.
 * 
 * By counting the number of detected rotations when rotating
 * the encoder one full rotation, we can determine the resolution
 * of the encoder, and the angle of rotation per pulse. We can also
 * add or subtract from the total value depending on which direction
 * of rotation occurs.
 */

#define encPinA 2
#define encPinB 3

byte poll(int PinA, int PinB) {
  //bool A = digitalRead(PinA);
  //bool B = digitalRead(PinB);
  //byte result = (byte)(B << 1) | (byte)A;
  bool A = (PinA == 1);
  bool B = (PinB == 1);
  byte result = (byte)(B << 1) | (byte)A;
  return result;
}

void setup() {
  Serial.begin(19200);
  Serial.println();
  pinMode(encPinA, INPUT_PULLUP);
  pinMode(encPinB, INPUT_PULLUP);
  Serial.println(poll(1,1));
}

void loop() {
  
}
