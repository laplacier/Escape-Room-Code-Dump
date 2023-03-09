| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-S2 | ESP32-S3 |
| ----------------- | ----- | -------- | -------- | -------- | -------- |

# twai_can component
Utilizes the built in twai_can functions of the ESP32 CAN controller to deliver beneficial information about the props in the network and full control of the props to the operator and technician.

## How to configure
This component can be configured by opening the ESP-IDF Configuration editor and navigating to "Prop CAN Options".

## Configuration Options
**Enable CAN** - TRUE by default. This will allow the prop to use CAN, which is the primary method for communicating between props. If CAN is disabled, the prop's default game stage will be 1 (Active) instead of 0 (Idle/Disabled) to prevent the prop from being unable to activate. You will need to be sure to develop your own method to reset the puzzle upon completion if CAN is disabled.

**CAN_TX GPIO Pin** - GPIO 33 by default. Can be any unused IO capable pin.

**CAN_RX GPIO Pin** - GPIO 32 by default. Can be any unused IO capable pin.

**CAN ID** - 1 by default. 0 is reserved for the game flow controller. Can assign any ID from 1 to 255. Each prop must use a unique ID to prevent conflicts on the CAN bus. The lowest ID on the CAN bus will have the highest priority when sending the same type of message.

**CAN operation mode**
    
Receive and Transmit - Normal operation. Can send and receive messages on the CAN bus.
    
Listen only - Can receive messages on the CAN bus and ACK them, but will not send messages or respond to message requests.

**Enable prop inheritance**

Allows the prop to participate in prop inheritance. When a device is sent a ping request multiple times without a ping reponse, any prop with this setting enabled will attempt to request to inherit the missing prop. If
the request is successful, the prop will "inherit" the send/receive responsibilities of the missing prop and
mimic their ID. This is useful for props that unexpectedly fail or disconnect from the CAN bus during a game to
allow smooth, continuous operation. If the inherited prop returns and sends a ping reponse, the prop will no 
longer be mimic'd.

## API reference
The following functions are included in [puzzle](puzzle) to utilize this component:

**bool CAN_Receive(uint32_t delay)**

Checks a queue which populates with messages sent to the prop ID for "delay" milliseconds converted from nonblocking FreeRTOS task ticks. The queue can contain an integer value which corresponds to the following messages:

```
0 - Change state
1 - GPIO mask of output pins to set
2 - GPIO pin states to set
3 - Play a sound file
4 - Shift register mask of output pins to set
5 - Shift register pin states to set
255 - Ping request
```

Each message is handled automatically by the function and transferred to its 
respective component. For option 1, change state, the global game state variable 
will be modified to the requested value. The twai_can components will be blocked 
from sending additional commands until the puzzle component calls and resolves 
the command in this function. If **rx_payload[]** is busy being written to by 
the CAN component, this function will be blocked from executing and return false.

**void CAN_Send_State(uint8_t target_id)**

Function called when replying to a ping request. Pushes the state of all of the 
active components in the prop through the CAN bus to prop with target_id. Avoid
firing this too in rapid succession to keep the CAN bus decongested.

**bool CAN_Send_Command(uint8_t target_id, uint8_t command)**

Sends a message on the CAN bus with the inputted command to the target_id of the 
prop. The same messages that can be received are the messages that can be sent. 
However, a payload corresponding to the command must be sent with the command. 
The payload must be written to the global variable **tx_payload[]** before 
calling this function. If **tx_payload[]** is busy being read by the CAN 
component, this function will be blocked from executing and return false.

```
// Change game state; Requires payload[0] to be written
tx_payload[0] = 1;     // Set game state to active/ready state
CAN_Send_Command(1,0); // Change game state of prop with ID 1

// Send GPIO output mask; Requires payload[0] thru payload[4] to be written
tx_payload[0] = 66;    // (0b01000011) Modify GPIO 0, 1, and 6
tx_payload[1] = 12;    // (0b00001100) Modify GPIO 10, 11
tx_payload[2] = 0;     // (0b00000000)
tx_payload[3] = 64;    // (0b01000000) Modify GPIO 30
tx_payload[4] = 0;     // (0b00000000)
CAN_Send_Command(1,1); // Send GPIO mask to prop with ID 1

// Send GPIO output state; Only the bits that were last masked matter
tx_payload[0] = 255;   // (0b11111111) Set all GPIO to HIGH (only 0,1,6 set)
tx_payload[1] = 0;     // (0b00000000) Set all GPIO to LOW  (only 10,11 set)
tx_payload[2] = 0;     // (0b00000000) Set all GPIO to LOW  (none set)
tx_payload[3] = 0;     // (0b00000000) Set all GPIO to LOW  (only 30 set)
tx_payload[4] = 255;   // (0b11111111) Set all GPIO to HIGH (none set)
CAN_Send_Command(1,2); // Send GPIO states to prop with ID 1

// Play a sound file; Requires payload[0] to be written
tx_payload[0] = 3;     // Select sound track 3
CAN_Send_Command(1,3); // Send sound track to play to prop with ID 1

// Send shift_reg output mask; Requires payload[0] thru payload[4] to be written
tx_payload[0] = 66;    // (0b01000011) Modify PISO 0 pins A, B, and G
tx_payload[1] = 12;    // (0b00001100) Modify PISO 1 pins C and D
tx_payload[2] = 0;     // (0b00000000)
tx_payload[3] = 64;    // (0b01000000) Modify PISO 3 pin G
tx_payload[4] = 0;     // (0b00000000)
CAN_Send_Command(1,4); // Send shift_reg mask to prop with ID 1

// Send shift_reg output state; Only the bits that were last masked matter
tx_payload[0] = 255;   // (0b11111111) Set all PISO 0 pins HIGH (only A,B,G set)
tx_payload[1] = 0;     // (0b00000000) Set all PISO 1 pins LOW  (only C,D set)
tx_payload[2] = 0;     // (0b00000000) Set all PISO 2 pins LOW  (none set)
tx_payload[3] = 0;     // (0b00000000) Set all PISO 3 pins LOW  (only G set)
tx_payload[4] = 255;   // (0b11111111) Set all PISO 4 pins HIGH (none set)
CAN_Send_Command(1,5); // Send GPIO states to prop with ID 1

CAN_Send_Command(1,255); // Send ping request to prop with ID 1; No payload
```