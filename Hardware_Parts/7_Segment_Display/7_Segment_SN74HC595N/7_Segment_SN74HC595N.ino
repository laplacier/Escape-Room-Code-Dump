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
 *  7-Segment Pinout            Pin to Segment Locations
 *                                    __________
 *       COM                         |    A     |
 *    G F | A B                   __ `----------' __
 *    |_|_|_|_|                  |  |            |  |
 *   |   ====  |                 | F|            |B |
 *   |  ||  || |                 |  | __________ |  |
 *   |   ====  |                 `--'|    G     |`--'
 *   |  ||  || |                  __ `----------' __
 *   |   ==== o|                 |  |            |  |
 *   `---------'                 | E|            |C |
 *    | | | | |                  |  | __________ |  |  __
 *    E D | C H                  `--'|    D     |`--' |H |
 *       COM                         `----------'     `--'
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
 *  Common anode  - The segments of the display turn on when COM is
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
 * created   9 Feb, 2023
 * modified 10 Feb, 2023
 */

#define COMMON_MODE 1     // 0 for Common Anode, 1 for Common Cathode
#define numSIPO     2     // Number of 7-segment displays and SIPO registers
#define latchPin    4     // Pin 12 on all SN74HC595N
#define clockPin    7     // Pin 11 on all SN74HC595N
#define dataOutPin  6     // Pin 14 on FIRST SN74HC595N
byte outputData[numSIPO]; // SIPO data to send and update 7-segment displays

/* 
 * Function: pulsePin
 * Accepts a digital output pin and a pulse time
 * Pulses a pin LOW for the specified pulse time
 * in microseconds.
 */
void pulsePin(int pinName, int pulseTime){
  digitalWrite(pinName, LOW);
  delayMicroseconds(pulseTime);
  digitalWrite(pinName, HIGH);
  delayMicroseconds(pulseTime);
}

/* 
 * Function: printData
 * Accepts a byte and the name of the register. Prints
 * the name of the register submitted and prints the byte
 * in the expected format returned from readPISO() or sent
 * to sendSIPO()
 */
void printData(byte data, String regName, int regNum){
  Serial.print("    ");                        
  Serial.print(regName);            // Print the register name
  Serial.print(regNum);             // Print the register number        
  Serial.print(": ");
  for (int j=0; j<8; j++){          // For each bit...
    Serial.print(bitRead(data, j)); // Print the data to/from the
    Serial.print(" ");              // shift register in LSBFIRST order
  }
  Serial.println();
}

/* 
 * Function: sendSIPO
 * Accepts a byte array in LSBFIRST order and writes the data to 
 * the SIPO registers.
 * 
 * Per the datasheet, data is written MSBFIRST, so the first bit
 * written will be written to pin H of the last SIPO register in 
 * the chain.
 * 
 * If you had two SIPO registers daisy chained, with 0A denoting
 * Pin A on the first SIPO register and 1A denoting Pin A on the
 * second SIPO register, the PHYSICAL order of pins being written
 * to would be the following:
 * 
 * Bit#: 00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15
 * Pin#: 1H 1G 1F 1E 1D 1C 1B 1A 0H 0G 0F 0E 0D 0C 0B 0A
 * 
 * However, this function reads the data from the byte array
 * in MSBFIRST order for ease of programming and readability.
 * 
 * Example:
 * Suppose you have two SIPO registers in a chain and you input the
 * following...
 * 
 * byte data = {127,1}; // (b01111111,b00000001)
 * 
 * The resulting output on the SIPO registers will be...
 * Pin States: A B C D E F G H
 *     SIPO#0: 0 1 1 1 1 1 1 1
 *     SIPO#1: 0 0 0 0 0 0 0 1
 */
void sendSIPO(byte data[numSIPO]){
  for(int i=numSIPO-1; i>=0; i--){                    // For each SIPO register...
    const String nameSIPO = "SIPO#";                  // Create register name
    printData(data[i], nameSIPO, i);                  // Print expected output to serial
    for(int j=7; j>=0; j--){                          // For each bit in the register...
       if(!COMMON_MODE)                               // If 7-segment display is in common anode configuration...
        data[i] = !data[i];                           // Invert the ON/OFF byte of the input variable
       digitalWrite(dataOutPin, bitRead(data[i], j)); // Read the current bit of the input variable and write it to the SIPO data pin
       pulsePin(clockPin, 5);                         // Pulse the clock to shift the bit out to SIPO registers
    }
  }
  pulsePin(latchPin, 5);                              // Pulse the latch to allow new data to appear on SIPO registers
}

/* 
 * Function: getDisplayByte
 * Accepts a character and a decimal flag. Uses a
 * switch case to determine which segments to turn on
 * based on the input character. For characters that
 * have no relevant display, nothing is shown.
 * 
 * Returns a byte of containing each segment state in
 * LSBFIRST order.
 */
byte getDisplayByte(char character, bool decimal){
  byte segments;                  // Stores the ON/OFF state of each segment to update
  bitWrite(segments, 7, decimal); // Write decimal flag to segments bit 7 (pin H)
  switch(character){              // Determine which segments to turn on based on the input character
    case '.': case ',':
      segments = 0b10000000; // In case a single decimal is passed, decimal ON
      break;
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
  return segments;           // Send the segment byte 
}

/* 
 * Function: updateDisplay
 * Accepts a character and bool, int, or string. Function 
 * overloaded to handle conversion of these type(s),
 * obtains relevant byte(s) from getDisplayByte() and sends 
 * byte(s) to SIPO register(s).
 *
 *     Input:     updateDisplay(1337);
 * Func Sees:      1   3   3   7 
 *  Displays:     [1 ][3 ][3 ][7 ]
 *  
 *     Input:     updateDisplay("HI",1);
 * Func Sees:          H   I 
 *  Displays:     [  ][H ][I ][  ]
 *
 * For inputs that are OOB, we only print characters that
 * are within the bounds of the display.
 *     
 *     Input:     updateDisplay("12FC5");
 * Func Sees:  1   2   F   C   5 
 *  Displays:     [2 ][F ][C ][5 ]
 *     
 *     Input:     updateDisplay(0.1234);
 * Func Sees:  0.  1   2   3   4 
 *  Displays:     [1 ][2 ][3 ][4 ]
 *     
 *     Input:     updateDisplay("HELP",-2);
 * Func Sees:              H   E   L   P 
 *  Displays:     [  ][  ][H ][E ]
 *     
 *     Input:     updateDisplay("yotE",1);
 * Func Sees:  y   o   t   E
 *  Displays:     [o ][t ][E ][  ]
 *     
 *     Input:     updateDisplay('A',4);
 * Func Sees:  A
 *  Displays:     [  ][  ][  ][  ]
 */
void updateDisplay(char character, byte pos=0, bool decimal=0){ // Update a single 7-segment display and decimal point
  if(pos <= numSIPO-1 && pos >=0){                              // If position to update is in bounds...
    outputData[pos] = getDisplayByte(character, decimal);       // Modify the specified 7-segment display
    sendSIPO(outputData);                                       // Send the changes to SIPO registers
  }
}
void updateDisplay(String message, byte pos=0){
  bool decimal = 0;
  for(int i=0; i<message.length(); i++){              // For each character in the string...
    char letter = message.charAt(i);                  // Store the character
    if(letter == '.')                                 // If character is a decimal...
      decimal = 1;                                    // Raise decimal flag, don't go to next segment
    else{                                             // Otherwise..
      updateDisplay(message.charAt(i), pos, decimal); // Send the character to be processed
      decimal = 0;                                    // Lower decimal flag
      pos++;                                          // Move to next segment
    }
  }
}
void updateDisplay(byte number, byte pos=0){
  updateDisplay(String(number), pos);  // Pass through to string function as string
}
void updateDisplay(int number, byte pos=0){
  updateDisplay(String(number), pos);  // Pass through to string function as string
}
void updateDisplay(double number, byte pos=0){
  updateDisplay(String(number), pos);  // Pass through to string function as string
}

void setup() {
  Serial.begin(19200);

  // Set all the pins of SN74HC595N as OUTPUT
  pinMode(latchPin, OUTPUT);
  pinMode(dataOutPin, OUTPUT);  
  pinMode(clockPin, OUTPUT);
}

void loop() {
  for(int i=0; i<10; i++){                         // For each number 0-9...
    Serial.println("Pin States: A B C D E F G H"); // Print header row in LSBFIRST order
    updateDisplay((char)i, i%numSIPO);             // Show numbers on displays, no decimal
    delay(500);                                    // Wait before showing next number
  }
  String message = "AbCdEFGH";                     // Create a string to update on the display
  Serial.println("Pin States: A B C D E F G H");   // Print header row in LSBFIRST order
  updateDisplay(message, 0);                       // Show character on display, no decimal
  delay(500);                                      // Wait before showing next character
}
