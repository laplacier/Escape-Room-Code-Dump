| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-S2 | ESP32-S3 |
| ----------------- | ----- | -------- | -------- | -------- | -------- |

# nfc component
Utilizes a pn5180 NFC reader from NXP to detect ISO15693 tags.

## How to configure
This component can be configured by opening the ESP-IDF Configuration editor and navigating to "NFC Options".

## Configuration Options
### Enable NFC
TRUE by default. This will enable the NFC capabilities of the prop.

### Detect multiple tags
FALSE by default. Multiple NFC tags can be placed on top of the NFC reader and read if enabled. Returns one NFC tag if only one present when false.

## API reference
The following functions can be called to utilize this component:

### CAN Messages
```

```
### bool shift_read(uint8_t pin)
Returns the value read from the specified pin on the input shift registers. Pins are specified in order from 0 *(reg#0, pinA)* to **NUM_SN74HC165n** - 1 *(reg#NUM_PISO, pinH)*.

### void shift_write(uint8_t pin, bool val)
Writes val to the specified pin on the output shift registers. Pins are specified in order from 0 *(reg#0, pinA)* to **NUM_SN74HC595n** - 1 *(reg#NUM_SIPO, pinH)*. Note that the changes will not be reflected until **shift_show()** is called.

### void shift_show(void)
Shows the changes on the output shift register pins written via **shift_write()**

## Example usage:
```
if(shift_read(12)){   // If input shift register 1, pin 4 is HIGH...
    shift_write(3,1); // Write HIGH to output shift register 0, pin 3
}
else{                 // Otherwise...
    shift_write(3,0); // Write LOW to output shift register 0, pin 3
}
shift_show();         // Show the state changes on output shift registers
```