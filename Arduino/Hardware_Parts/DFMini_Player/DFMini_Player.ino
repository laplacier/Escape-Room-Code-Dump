/*
 * DFMini Player DEMO
 * 
 * We have a prop, and we'd like that prop to play some sound 
 * clips or music when an interaction occurs. There are options
 * available out there, but they're  either incredibly expensive,  
 * sometimes costing upwards in the hundred of dollars, or limited 
 * in how we can activate them and all we want to do is play a 
 * few sound clips...
 * 
 * For as low as $2, we can obtain a DFMini Player! This little 
 * IC accepts a microSD card containing MP3 audio and plays the
 * audio on two speaker pins when given a command via serial 
 * from a microcontroller. We can have sound on the cheap AND
 * have near limitless ability to decide when and how it plays
 * sound.
 * 
 * Use a DFMini Player:
 * -For playing background music
 * -To play music or a sound due to an observable trigger
 * 
 * Do NOT use a DFMini Player:
 * -Directly in "Higher" low voltage systems. 3.3V and 5V only!
 * -Directly with a speaker greater than 3W. Larger speakers require physical amplification.
 * -In props with parts and software that require fast operation, like polling encoders.
 *  Serial takes time, and the DFMini communicates at a slow baud rate!
 *  
 * We will need two serial compatible IO pins and a 3W or smaller speaker to operate the
 * DFMini Player. Only one GND is required, but it is safer to connect both.
 *                            ________ ________
 *                           |        U        |
 *                       VCC-|1    DFMini    16|-
 *                        RX-|2    Player    15|-
 *                        TX-|3  ___________ 14|-
 *                          -|4 |           |13|-
 *                          -|5 |           |12|-
 *                     +SPKR-|6 |           |11|-
 *                       GND-|7 |           |10|-GND
 *                     -SPKR-|8 |___________| 9|-
 *                           |_________________| 
 * 
 * The following rules must be adhered to when placing sound files on the microSD card for
 * the DFMini Player:
 * 
 * -The microSD card total capacity cannot exceed 32GB.
 * -The microSD card file system format must be FAT16 or FAT32.
 * -The sound file must be in MP3 format. The official DFMini Player supports WAV and WMA,
 *  but the clones struggle encoding these.
 * -The files must be placed in a folder named "MP3" without quotes, and that folder must
 *  be placed in the root directory of the microSD card.
 * -The files must be renamed to a 4 digit number in the format (0001-2999). For example,
 *  0001.mp3.
 * 
 * by Adam Billingsley
 * created 8 Feb, 2023
 */

#include <SoftwareSerial.h>                    // Allows serial communication on compatible IO pins

#define DEBUG true                             // Prints debug information to serial if true
#define DFMINI_TX 10                           // TX pin of the DFMini. Connects to microcontroller RX pin.
#define DFMINI_RX 11                           // RX pin of the DFMini. Connects to microcontroller TX pin.
#define DFMINI_VOLUME 20                        // Software configurable volume. Max volume = 30.
SoftwareSerial DFMini(DFMINI_TX, DFMINI_RX);   // Declare a serial object named DFMini and pass the serial pins

/*
 * Function: playTrack
 * This function simplifies sending a "play audio" command by accepting a
 * track number and sending the correct request to the DFMini Player with the 
 * requested track number. If sent while another track is already playing,
 * the previous track will immediately end and the current track will begin
 * playing.
 * 
 * Example: playTrack(4) will play 0004.mp3 located in the MP3 folder.
 */
void playTrack(word trackNum){
  sendAudioCommand(0x03,trackNum); // Send command 0x03 (play track#) and parameter trackNum (track# to play)
}

/*  
 *  Function: sendAudioCommand
 *  This function accepts a command byte and 2-byte parameter, constructs
 *  an appropriately formatted serial instruction for the DFMini Player,
 *  and sends the command over serial.
 *  
 *  A correct serial instruction consists of the following:
 *  
 *  Name  $S | VER| LEN| CMD| FB | P1 | P2 |CKSM|CKSL| $O
 *       -------------------------------------------------
 *  Bytes 7E | FF | 06 | XX | 0X | XX | XX | XX | XX | EF
 *  
 *   $S - Start byte, the start of instruction to DFMini Player. Static value of 0x7E.
 *  VER - Version byte, needed, but purpose unclear. Static value of 0xFF.
 *  LEN - Length byte, the number of bytes after LEN and before CKSM. Static value of 0x06.
 *  CMD - Command byte, the actual action the DFMini must perform. Relevant bytes are listed below.
 *   FB - Feedback byte, a flag that enables receiving debug information back from the DFMini Player in response.
 *   P1 - Parameter byte, the most significant byte. Used to select an option for commands with multiple options.
 *   P2 - Parameter byte, the least significant byte. Used to select an option for commands with multiple options.
 * CKSM - Checksum byte, the most significant byte. Verifies the previous data contains no errors when received.
 * CKSL - Checksum byte, the least significant byte. Verifies the previous data contains no errors when received.
 *   $O - End byte, the end of instruction to the DFMini Player. Static value of 0xEF.
 * 
 * For the purposes of explaining the creation of the checksum, suppose we want to play track 0001.mp3, 
 * thus sending a command byte of 0x03 and a two-byte parameter of 0x0001.
 * 
 * To create the two byte checksum, first add the version byte, length byte, command byte, feedback
 * byte, most significant parameter byte, and least significant parameter byte together.
 * 
 *                                (VER) 0xFF +
 *                                (LEN) 0x06 +
 *                                (CMD) 0x03 +
 *                                ( FB) 0x00 +
 *                                ( P1) 0x00 +
 *                                ( P2) 0x01 = 0x0109
 * 
 * The two-byte checksum equals the inverse of the resulting value 0x0109, which equals 0xFEF7.
 * 
 * Below is a table containing commands and their associated valid parameters. There are more commands
 * available to use, but these are the primary commands used to make effective use of the DFMini Player
 * to use with props.
 *                      _________________________________________
 *                     | byte CMD |   COMMAND   | PARAMETER VALS |
 *                     |----------|-------------|----------------|
 *                     |   0x03   | Play track# | 0-2999         |
 *                     |   0x06   | Set volume  | 0-30           |
 *                     |   0x07   |   Set EQ    | Normal(0),Pop(1),Rock(2),Jazz(3),Classic(4),Bass(5)
 *                     |   0x08   | Player mode | Repeat all(0),Repeat folder(1),??Repeat next track(2)??,Play random tracks(3)
 *                     |   0x0A   | Sleep mode  | 0              |
 *                     |   0x0B   |  Wake up    | 0              |
 *                     |   0x0C   |Reset DFMini | 0              |
 *                     |   0x0D   |Enable DFMini| 0              |
 *                     |   0x0E   | Pause track | 0              |
 *                     |   0x11   | Loop track  | Disable(0),Enable(1)
 *                      -----------------------------------------
 */
void sendAudioCommand(byte command, word parameter){
  //------------------ CREATE INSTRUCTION -------------------------//
  byte startByte     = 0x7E; // Start
  byte versionByte   = 0xFF; // Version
  byte commandLength = 0x06; // Length
  byte feedback      = 0x00; // Feedback
  byte endByte       = 0xEF; // End

  word checksum = -(         // Create the two-byte checksum
    versionByte +
    commandLength +
    command +
    feedback +
    highByte(parameter) +
    lowByte(parameter));
  
  byte instruction[10] = {   // Create a byte array of the instruction to send 
    startByte,
    versionByte,
    commandLength,
    command,
    feedback,
    highByte(parameter),
    lowByte(parameter),
    highByte(checksum),
    lowByte(checksum),
    endByte
    };
    
  //------------------- SEND INSTRUCTION --------------------------//
  if(DEBUG){
    Serial.println("           |STRT|VERS| LEN| CMD| FB |PAR1|PAR2|CKM1|CKM2| END|");
    Serial.print("INSTR SENT:|");
  }
  for (byte clb=0; clb<10; clb++){       // For each byte in the instruction...
    DFMini.write(instruction[clb]);      // Send the selected byte to the DFMini Player via serial
    if(DEBUG){
      if(instruction[clb] < 15){
        Serial.print(" 0x");
      } else{
        Serial.print("0x");
      }
      Serial.print(instruction[clb],HEX);
      Serial.print("|");
    }
  }
  if(DEBUG)
    Serial.print("\r\n\r\n");
  delay(100);                            // Wait for the DFMini Player to process the instruction
}

void setup() {
  Serial.begin(19200);

  DFMini.begin(9600);                    // Begin serial communication with DFMini Player at the required 9600 baud
  delay(1000);                           // Allow DFMini Player time to detect serial. Clones depend on a delay.
  sendAudioCommand(0x0D,0);              // Send command 0x0D (Enable DFMini Player) and no parameter required
  sendAudioCommand(0x09,0);
  sendAudioCommand(0x07,0);              // Send command 0x07 (Set EQ) and parameter 0x00 (Normal EQ)
  sendAudioCommand(0x06, DFMINI_VOLUME); // Send command 0x06 (Set Volume) and parameter DFMINI_VOLUME (defined)
}

void loop() {
  playTrack(1); // Play 0001.mp3
  delay(5000);  // Allow 0001.mp3 to play for a bit
  //playTrack(2); // Play 0002.mp3
  //delay(5000);  // Allow 0002.mp3 to play for a bit
}
