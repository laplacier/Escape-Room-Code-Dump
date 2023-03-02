/*
 * DEBOUNCE DEMO
 * 
 * When a button is pressed, the press can be detected and
 * used to trigger additional actions. Simple!
 *                      ______
 *                     (______)
 *                     |  ==  | 
 *                     |  ==  |
 *                     '------'
 *                      |    |
 *          Input(PULLUP)     GND
 * 
 * We will need 1 input pin to read a button. If the pin
 * on the microcontroller does not support INPUT_PULLUP, 
 * we will need to add an external pullup resistor.
 * 
 * With cheap or gently used buttons, we will experience an
 * issue where a single button press is seen as multiple
 * presses by the microcontroller. This issue is called
 * bouncing, where the conductive mechanism inside the button
 * bounces rapidly in a small timeframe on a single press, 
 * opening and closing the circuit several times. This
 * can cause unintended behavior in our props.
 * 
 * We can eliminate this issue by utilizing debouncing.
 * For a specified amount of time in milliseconds, we can
 * ignore any state changes after the initial change, allowing
 * the internal conductive mechanism time to stop bouncing.
 * 
 * by Adam Billingsley
 * created 8 Feb, 2023
 */

#define buttonPin      1   // Any leg of the button, buttons are bidirectional.
#define debounceTime   100 // The time, in milliseconds, to disallow a button state from being acted upon after a button state was already acted upon.
long lastButtonTime;       // Snapshots the last time a button state was acted upon.
bool buttonTriggered = 0;  // Flag stores the current held toggle state.

/* 
 * Function: isDebounced
 * Returns true if debounceTime milliseconds have passed
 * since the last button press was triggered
 */
bool isDebounced(){
  return (millis() - lastButtonTime) > debounceTime;
}

void setup() {
  Serial.begin(19200);
  pinMode(buttonPin, INPUT_PULLUP); // Button pin is INPUT_PULLUP
  lastButtonTime = millis();        // Initialize debounce timer
}

void loop() {
  bool button = !digitalRead(buttonPin);                // Get the state of the button. Inverted so that 1 = pressed, 0 = not pressed
  
  if(isDebounced() && button && !buttonTriggered){      // If the button is pressed and the debounce time has passed...
                                                        // Do a thing in response!
    Serial.println("You pressed the button!");
    buttonTriggered = 1;                                // Flag the button as pressed, do not let this fire again until button is released later
    lastButtonTime = millis();                          // Reset the debounce timer
  }
  else if(isDebounced() && !button && buttonTriggered){ // If the button is released and the debounce time has passed...
                                                        // Do a thing in response!
    Serial.println("Button released.");
    buttonTriggered = 0;                                // Flag the button as released, do not let this fire again until button is pressed later
    lastButtonTime = millis();                          // Reset the debounce timer
  }
}
