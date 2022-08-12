/*
 * A demo of a simplest AceButton that has a visible effect. One button is
 * connected to the digital pin BUTTON_PIN. It uses the internal pull-up
 * resistor (INPUT_PULLUP). Pressing the button turns on the built-in LED.
 * Releasing the button turns off the LED.
 */
#include <Arduino.h>
#include <button.h>

// object button
struct buttonT{
int buttonPin = BUTTON_PIN;
int lastButtonState;
unsigned int lastDebounceTime;
unsigned int debounceDelay = 100;
bool buttonState ;
bool buttonPressed = false;
int buttonStart;
int buttonEnd; 
int buttonLongDelay = 2000;
char buttonEvent;
} button1;


char checkButton(){ // read the state of the switch into a local variable:
    
    button1.buttonEvent = NO_CHANGE;

    int reading = digitalRead(button1.buttonPin);
    // If the switch changed, due to noise or pressing:
    if (reading != button1.lastButtonState) {
      // reset the debouncing timer
      button1.lastDebounceTime = millis();
    }

    if ((millis() - button1.lastDebounceTime) > button1.debounceDelay) {
      // whatever the reading is at, it's been there for longer than the debounce
      // delay, so take it as the actual current state:

      // if the button state has changed:
      if (reading != button1.buttonState) {
        
        button1.buttonState = reading;

        // only toggle the LED if the new button state is HIGH
        if (button1.buttonState == LOW) {
          button1.buttonPressed = true;
          button1.buttonStart = millis();
          button1.buttonEnd = button1.buttonStart;
        }
        // button just got released
        else{
          if(millis() - button1.buttonStart > button1.buttonLongDelay){
              // do long delay task
            button1.buttonEvent = LONG_PRESS;
        }
        else{
          // short delay so just change page  
            button1.buttonEvent = SHORT_PRESS;
          }
              
        }
      }
    }
    // save the reading. Next time through the loop, it'll be the lastButtonState:
    button1.lastButtonState = reading;
    return button1.buttonEvent;
}

    
