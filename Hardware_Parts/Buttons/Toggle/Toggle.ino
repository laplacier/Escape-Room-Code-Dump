/*
 * BUTTON TOGGLE DEMO
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
 * But what if, on that press, we wanted to treat the action
 * as a toggle? Buttons exist that physically toggle and hold
 * their pressed or released state. However we can accomplish 
 * the same task with a regular button and some programming!
 * 
 * We might also have code that needs to run exactly once on
 * a button toggle. We will handle this case as well.
 * 
 * We will need 1 input pin to read a button. If the pin
 * on the microcontroller does not support INPUT_PULLUP, 
 * we will need to add an external pullup resistor.
 * 
 * by Adam Billingsley
 * created 8 Feb, 2023
 */

#define buttonPin      1                 // Any leg of the button, buttons are bidirectional.
bool buttonToggled =   0;                // Flag stores the current held toggle state.

void setup() {
  Serial.begin(19200);
  pinMode(buttonPin, INPUT_PULLUP);      // Button pin is INPUT_PULLUP
}

void loop() {
  bool button = !digitalRead(buttonPin); // Get the state of the button. Inverted so that 1 = pressed, 0 = not pressed
  
  if(button && !buttonTriggered){        // If the button is pressed...
    buttonTriggered = 1;                 // Flag the button as pressed, do not let this fire again until button is released later
    buttonToggled ^= 0;                  // Change the toggle state of the button to the opposite state
    flag_toggleOnce = 1;                 // Allow code that triggers once when button is toggled to execute
    if(buttonToggled){                   // If the toggle state of the button is "pressed"...
                                         // Do a thing once for a "pressed" toggle button
      Serial.println("Button toggled, holding pressed!");
    }
    else{                                // If the toggle state of the button is "unpressed"...
                                         // Do a thing once for an "unpressed" toggle button
      Serial.println("Button toggled, released.");    
    }
  }
  else if(!button && buttonTriggered){   // If the button is released...
    buttonTriggered = 0;                 // Flag the button as released, do not let this fire again until button is pressed later
  }

  if(buttonToggled){                     // If the toggle state of the button is "pressed"...
                                         // Do a thing indefinitely while held pressed
  }
  else{                                  // If the toggle state of the button is "unpressed"...
                                         // Do a thing indefinitely while unpressed
  }
}
