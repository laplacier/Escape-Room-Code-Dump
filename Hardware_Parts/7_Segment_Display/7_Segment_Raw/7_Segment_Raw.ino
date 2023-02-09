/*
 * 7-SEGMENT DISPLAY WITH RAW IO DEMO
 * 
 * We have numeric information we would like to display on our 
 * prop, and we want the display to fit the theme of our prop, 
 * which is in a digital retro style. 7-segment displays are 
 * great devices to fit this purpose and style!
 * 
 * Use 7-segment displays for:
 * -Fitting a characteristic theme
 * -Displaying numerical information in a static, nostalgic font
 * -Providing numerical feedback from microcontrollers for
 *  visual debugging, when serial access is impractical or
 *  difficult.
 * 
 * Do NOT use 7-segment displays:
 * -In RAW IO implementation when IO pins are limited
 * -To display full strings. A few letters are plausible, 
 *  but not all.
 * 
 * 7-segment displays, in a nutshell, are 7 LEDs in parallel.
 * Each segment of a 7-segment display is a single LED. That
 * means we can control each segment the same way we would
 * toggle an LED on and off!
 * 
 * 7-Segment Pinout              Pin to Segment Locations
 *                                    ______________
 *       COM                         |      A       |
 *    G F | A B                   __ `--------------' __
 *    |_|_|_|_|                  |  |                |  |
 *   |   ====  |                 | F|                |B |
 *   |  ||  || |                 |  | ______________ |  |
 *   |   ====  |                 `--'|      G       |`--'
 *   |  ||  || |                  __ `--------------' __
 *   |   ==== o|                 |  |                |  |
 *   `---------'                 | E|                |C |
 *    | | | | |                  |  | ______________ |  |  __
 *    E D | C H                  `--'|      D       |`--' |H |
 *       COM                         `--------------'     `--'
 * 
 * 7-segment displays come in two configurations which are important
 * to take note of when connecting and operating the display. We can
 * determine the configuration of the display by attempting the two
 * tests below:
 * 
 * Common cathode - The segments of the display turn on when COM is
 *                  connected to GND and the segment is pulled HIGH.
 *                                
 *                              Segment A        
 *                                |  /|
 *                                | / |
 *                                |/  |
 *                COM(GND) -------<LED|------- A(HIGH)
 *                                |\  |
 *                                | \ |
 *                                |  \|
 *                                
 *   Common anode - The segments of the display turn on when COM is
 *                  connected to VCC and the segment is pulled LOW.
 *                                
 *                              Segment A    
 *                                |\  |
 *                                | \ |
 *                                |  \|
 *                COM(VCC) -------|LED>------- A(LOW)
 *                                |  /|
 *                                | / |
 *                                |/  |
 * 
 * We will need 8 IO pins to control a single 7-segment display
 * directly from the microcontroller, which is a RAW method. This
 * is not the recommended way to control a 7-segment display due to
 * the large amount of IO pins required to control a single display, 
 * but this method is provided for learning purposes.
 * 
 * by Adam Billingsley
 * created 9 Feb, 2023
 */

#define COMMON_MODE 1 // 0 for Common Anode, 1 for Common Cathode
#define pinA        1 // Segment A
#define pinB        2 // Segment B
#define pinC        3 // Segment C
#define pinD        4 // Segment D
#define pinE        5 // Segment E
#define pinF        6 // Segment F
#define pinG        7 // Segment G
#define pinH        8 // Segment H

void setup() {
  Serial.begin(19200);
  
  // Set all the pins of 7-segment display as OUTPUT
  pinMode(pinA, OUTPUT);
  pinMode(pinB, OUTPUT);
  pinMode(pinC, OUTPUT);
  pinMode(pinD, OUTPUT);
  pinMode(pinE, OUTPUT);
  pinMode(pinF, OUTPUT);
  pinMode(pinG, OUTPUT);
  pinMode(pinH, OUTPUT);
}

/* 
 * Function: updateDisplay
 * Accepts a character and a decimal flag. Uses a
 * switch case to determine which segments to turn on
 * based on the input character. For characters that
 * have no relevant display, nothing is shown.
 */
void updateDisplay(char character, bool decimal){  
  if(decimal)                // If the decimal needs to turn on...
    digitalWrite(pinH, ON);  // Turn on the decimal
  else                       // Otherwise...
    digitalWrite(pinH, OFF); // Turn off the decimal

  switch(character){         // Determine which segments to turn on based on the input character
    case '0': case 'O': case 'D':
      segments = 0b0111111;  // F,E,D,C,B,A ON
      break;
    case '1': case 'I': case 'l':
      segments = 0b0000110;  // C,B ON
      break;
    case '2': case 'Z':
      segments = 0b1011011;  // G,E,D,B,A ON
      break;
    case '3':
      segments = 0b1001111;  // G,D,C,B,A ON
      break;
    case '4': case 'Y': case 'y':
      segments = 0b1100110;  // G,F,C,B ON
      break;
    case '5': case 'S':
      segments = 0b1101101;  // G,F,D,C,A ON
      break;
    case '6': case 'G':
      segments = 0b1111101;  // G,F,E,D,C,A ON
      break;
    case '7':
      segments = 0b0000111;  // C,B,A ON
      break;
    case '8': case 'B':
      segments = 0b1111111;  // G,F,E,D,C,B,A ON
      break;
    case '9': case 'g':
      segments = 0b1101111;  // G,F,D,C,B,A ON
      break;
    case 'A': case 'R':
      segments = 0b1110111;  // G,F,E,C,B,A ON
      break;
    case 'b':
      segments = 0b1111100;  // G,F,E,D,C ON
      break;
    case 'C':
      segments = 0b0111001;  // F,E,D,A ON
      break;
    case 'c':
      segments = 0b1011000;  // G,E,D ON
      break;
    case 'd':
      segments = 0b1011110;  // G,E,D,C,B ON
      break;
    case 'E':
      segments = 0b1111001;  // G,F,E,D,A ON
      break;
    case 'F': case 'f':
      segments = 0b1110001;  // G,F,E,A ON
      break;
    case 'H': case 'X':
      segments = 0b1110110;  // G,F,E,C,B ON
      break;
    case 'h':
      segments = 0b1110100;  // G,F,E,C ON
      break;
    case 'i':
      segments = 0b0000100;  // C ON
      break;
    case 'J': case 'j':
      segments = 0b0011110;  // E,D,C,B ON
      break;
    case 'L':
      segments = 0b0111000;  // F,E,D ON
      break;
    case 'n':
      segments = 0b1010100;  // G,E,C ON
      break;
    case 'o':
      segments = 0b1011100;  // G,E,D,C ON
      break;
    case 'P': case 'p':
      segments = 0b1110011;  // G,F,E,B,A ON
      break;
    case 'q':
      segments = 0b1100111;  // G,F,C,B,A ON
      break;
    case 'r':
      segments = 0b1010000;  // G,E ON
      break;
    case 't':
      segments = 0b1111000;  // G,F,E,D ON
      break;
    case 'U': case 'V':
      segments = 0b0111110;  // F,E,D,C,B ON
      break;
    case 'u': case 'v':
      segments = 0b0011100;  // E,D,C ON
      break;
    default:
      break;
  }

  // Use predefined COMMON_MODE to determine whether to pull the pins HIGH or LOW
  bool ON  =  COMMON_MODE;                // HIGH for Common Cathode, LOW for Common Anode
  bool OFF = !COMMON_MODE;                // LOW for Common Cathode, HIGH for Common Anode
  byte segments = 0b0000000;              // Initialize all segments to OFF (0)
  byte segPin[7] = {pinA,pinB,pinC,pinD,  // Store segment pins in array for the following loop
                    pinE,pinF,pinG}; 
                    
  for(int i=0; i<7; i++){                 // For each segment...
    if(bitRead(segments,i))               // If the segment is flagged ON...
      digitalWrite(segPin[i], ON);        // Turn on the segment
    else                                  // Otherwise...
      digitalWrite(segPin[i], OFF);       // Turn off the segment
  }
}

void loop() {
  for(int i=0; i<10; i++){     // For each number 0-9...
    updateDisplay((char)i, 0); // Show number on display, no decimal
    delay(500);                // Wait before showing next number
  }
  String message = "AbCdEFGH";           // Create a string to update on the display
  for(int i=0; i<message.length(); i++){ // For each character in the string...
    updateDisplay(message.charAt(i), 0); // Show character on display, no decimal
    delay(500);                          // Wait before showing next character
  }
}
