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
 * -The sound file must be in MP3 format.
 * -The files must be placed in a folder named "MP3" without quotes, and that folder must
 *  be placed in the root directory of the microSD card.
 * -The files must be renamed to a 4 digit number in the format (0001-2999). For example,
 *  0001.mp3.
 * 
 * by Adam Billingsley
 * created 8 Feb, 2023
 */

#include <SoftwareSerial.h>                  // Allows serial communication on compatible IO pins

#define DFMINI_TX 10                         // TX pin of the DFMini. Connects to microcontroller RX pin.
#define DFMINI_RX 11                         // RX pin of the DFMini. Connects to microcontroller TX pin.
#define AUDIO_VOLUME 5                       // Software configurable volume. Max volume = 30.
SoftwareSerial DFMini(DFMINI_TX, DFMINI_RX); // Declare a serial object named DFMini and pass the serial pins

/*
 * Function: playTrack
 * This function simplifies sending a "play audio" command by accepting a
 * track number and sending the correct request to the DFMini Player with the 
 * requested track number. 
 * 
 * Example: playTrack(4) will play 0004.mp3 located in the MP3 folder.
 */
void playTrack(word trackNum){
  sendAudioCommand(0x03,trackNum); // Send command 0x03 (play track#) and parameter trackNum (track# to play)
}

/*  
 *  Function: sendAudioCommand
 *  TODO
 *  _________________________________________
 * | byte CMD |   COMMAND   | PARAMETER VALS |
 * |----------|-------------|----------------|
 * |   0x03   | Play track# | 0-2999         |
 * |   0x06   | Set volume  | 0-30           |
 * |   0x07   |   Set EQ    | Normal(0),Pop(1),Rock(2),Jazz(3),Classic(4),Bass(5)
 * |   0x08   | Player mode | Repeat all(0),Repeat folder(1),??Repeat next track(2)??,Play random tracks(3)
 * |   0x0A   | Sleep mode  | 0              |
 * |   0x0B   |  Wake up    | 0              |
 * |   0x0C   |Reset DFMini | 0              |
 * |   0x0D   |Enable DFMini| 0              |
 * |   0x0E   | Pause track | 0              |
 * |   0x11   | Loop track  | Disable(0),Enable(1)
 *  -----------------------------------------
 */
void sendAudioCommand(byte command, word parameter){
  // The following byte values come from DF Robot:
  byte startByte     = 0x7E;
  byte versionByte   = 0xFF;
  byte commandLength = 0x06;
  byte acknowledge   = 0x00;
  byte endByte       = 0xEF;

  word checksum = -(
    versionByte +
    commandLength +
    command +
    acknowledge +
    highByte(parameter) +
    lowByte(parameter));
  
  byte commandLine[10] = {
    startByte,
    versionByte,
    commandLength,
    command,
    acknowledge,
    highByte(parameter),
    lowByte(parameter),
    highByte(checksum),
    lowByte(checksum),
    endByte
    };

  for (byte clb=0; clb<10; clb++){
    DFMini.write(commandLine[clb]);
  }
  delay(100);
}

void setup() {
  Serial.begin(19200);

  DFMini.begin(9600);                   // initialize dfmini
  delay(1000);
  sendAudioCommand(0x0D,0);             // Send command 0x03 (play track#) and parameter trackNum (track# to play)
  sendAudioCommand(0x07,0);             // set EQ to normal
  sendAudioCommand(0x06, AUDIO_VOLUME); // Set the 
}

// the loop function runs over and over again forever
void loop() {
  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  digitalWrite(RELAY, HIGH);
  playTrack(1);
  delay(5000);                       // wait for a second
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
  digitalWrite(RELAY, LOW);
  playTrack(2);
  delay(5000);                       // wait for a second
}
