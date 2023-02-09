/*
 * 7-SEGMENT DISPLAY WITH SN74HC595N DEMO
 * 
 * SIPO - Serial in, parallel out. The SN74HC595N is a SIPO 
 * register. It accepts 1 serial input byte and outputs each bit 
 * to 8 outputs in parallel.
 * 
 * We have numeric information we would like to display on our 
 * prop, and we want the display to fit the theme of our prop, 
 * which is in a digital retro style. 7-segment displays are 
 * great devices to fit this purpose and style!
 * 
 * However, this requires a lot of digital output connections to 
 * make, but we don't have enough pins on our microcontroller to 
 * connect all the pins of the display(s). Rather than buying a 
 * more expensive microcontroller with more pins, we can add some 
 * inexpensive SIPO registers!
 * 
 * Use 7-segment displays for:
 * -Fitting a characteristic theme
 * -Displaying numerical information in a static, nostalgic font
 * -Providing numerical feedback from microcontrollers for
 *  visual debugging, when serial access is impractical or
 *  difficult.
 * 
 * Do NOT use 7-segment displays:
 * -To display full strings. A few letters are plausible, 
 *  which are included in this example, but not all.
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
 * 
 * We will need 3 IO pins to control any amount of SIPO
 * registers daisy chained together, and one SIPO register
 * per 7-segment display. The last SIPO register in the 
 * chain will have nothing connected to pin 9.
 * 
 * SIPO#0 __ __                      SIPO#1 __ __
 *       |  U  |                           |  U  |
 *     B-|1  16|-VCC                    B'-|1  16|-VCC
 *     C-|2  15|-A                      C'-|2  15|-A'
 *     D-|3  14|-DATA                   D'-|3  14|- to SIPO#0 pin 9
 *     E-|4  13|-GND                    E'-|4  13|-GND
 *     F-|5  12|-LATCH                  F'-|5  12|-LATCH
 *     G-|6  11|-CLK                    G'-|6  11|-CLK
 *     H-|7  10|-VCC                    H'-|7  10|-VCC
 *   GND-|8   9|- to SIPO#1 pin 14     GND-|8   9|- to SIPO#2 pin 14, etc...
 *       |_____|                           |_____|
 *       
 * by Adam Billingsley
 * created 9 Feb, 2023
 */

#define COMMON_MODE 1 // 0 for Common Anode, 1 for Common Cathode
#define numSIPO     1 // Number of 7-segment displays and SIPO registers
#define latchPin    4 // Pin 12 on all SN74HC595N
#define clockPin    7 // Pin 11 on all SN74HC595N
#define dataOutPin  6 // Pin 14 on FIRST SN74HC595N

/* 
 * Function: getDisplayByte
 * Accepts a character and a decimal flag. Uses a
 * switch case to determine which segments to turn on
 * based on the input character. For characters that
 * have no relevant display, nothing is shown.
 */
byte getDisplayByte(char character, bool decimal){
  byte segments;             // Stores the ON/OFF state of each segment to update
  bitWrite(segments, 7, decimal); // Write decimal flag to segments bit 7 (pin H)
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
      segments = 0b0000000;  // Unknown, all OFF
      break;
  }

  // Use predefined COMMON_MODE to determine whether to pull the pins HIGH or LOW
  bool ON  =  COMMON_MODE;                // HIGH for Common Cathode, LOW for Common Anode
  bool OFF = !COMMON_MODE;                // LOW for Common Cathode, HIGH for Common Anode
                    
  sendSIPO(segments);
  }
}

void setup() {
  Serial.begin(19200);

  // Set all the pins of SN74HC595N as OUTPUT
  pinMode(latchPin, OUTPUT);
  pinMode(dataOutPin, OUTPUT);  
  pinMode(clockPin, OUTPUT);
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
