#include <Keyboard.h>

// This firmware is made for running on a Pro Micro.
// Ports are hard-coded due to irregular pinout (i.e. can't use all portd due to reserved pins for LEDs).

// By default, the firmware supports up to 10 inputs, Arduino pins 0-9.

// KEYS

// 0 = D2
// 1 = D3
// 2 = D1
// 3 = D0
// 4 = D4
// 5 = C6
// 6 = D7
// 7 = E6
// 8 = B4
// 9 = B5

// CONFIGURATION SECTION
// Change values to fit your needs

// Lighting pins

// 10 = Reserved for lighting effects.
// Feed LEDs from 5V, sink to pin 10.
// If lighting current exceeds 20mA, use a transistor.


// Change this to the keycount.
// Valid range is 1-10
const int KEYCOUNT = 4;

// Change this to key debouncing time.
// Default for Cherry MX style switches is usually 5ms.
const int DEBOUNCETIME = 5;

// Keymap of chars to be pressed, from keys 0 to 9, layer 1 on top.
// For a function key to 2nd layer, use the ^ karat.
// Any keys below a karat are ignored.
// For a LED toggle key, use the $ char.
// Null chars do not send anything.

const char KEYMAP[2][10] = {
  {'z', 'x', KEY_ESC, '^', 0, 0, 0, 0, 0, 0 },
  {KEY_TAB, '$', '`', '^', 0, 0, 0, 0, 0, 0 }
};

// From here and below is the actual code
// Only modify if you want to change the ports and stuff

// Status: Array of ints from 0 to 2.
// 0 = Key unpressed
// 1 = Key pressed
// 2 = Key released, 5ms debounce cooldown
int keyStatus[KEYCOUNT];

// Read: Raw read values.
// Used for matrix scanning.
int keyRead[KEYCOUNT];

// Debouncing array.
unsigned long lastRelease[KEYCOUNT];

// Current pressed keys
bool keyTracker[2][KEYCOUNT];

// Current layer
int layer;

// LED status
bool ledOn;

// LED brightness, 0-255
int ledBrightness;

// LED Wave Value
int ledWave;


void setup() {
  // Set all pins 0-Keycount to input
  // Also, init variables
  for (int i = 0; i < KEYCOUNT; i++) {
    pinMode(i, INPUT_PULLUP);
    keyStatus[i] = 0;
    keyRead[i] = 0;
    lastRelease[i] = millis();
    keyTracker[0][i] = false;
    keyTracker[1][i] = false;
  }
  // Set LED pin
  pinMode(10, OUTPUT);

  // Define all remaining variables
  layer = 0;
  ledOn = true;
  ledBrightness = 30;
  ledWave = 20;
}

bool anyKeyPressed() {
  for (int i = 0; i <= 1; i++) {
    for (int j = 0; j < KEYCOUNT; j++) {
      if (keyTracker[i][j] == true) {
        return true;
      }
    }
  }
  return false;
}

void toggleLED() {
  ledOn = !ledOn;
  if (!ledOn) {
    ledBrightness = 0;
  }
}

void calculateLED() {
  // Return if LED is off
  if (!ledOn) {
    return;
  }
  
  // We use a sine wave to calculate lighting
  // Equation for idle breathing LED is
  // brightness = 25sin(seconds) + 50;
  ledWave = 25 * sin(millis() * 1000) + 50;

  // If any key is pressed, max brightness mode
  if (anyKeyPressed()) {
    ledBrightness = 255;
  }
  // Otherwise, fade until it's lower than ledWave, at which point it merges in with ledWave
  else {
    if (ledBrightness < ledWave) {
      ledBrightness = ledWave;
    }
    else {
      ledBrightness--;
    }
  }
  // Finally, set the pin
  // The pin SINKS current rather than OUTPUTS.
  // This is for stock transistor compatibility.
  // Make sure LEDs flow in from 5V into the LED pin with plenty of resistance!
  analogWrite(10, 255 - ledBrightness);
  
}

void scanKeys() {
  // Loop through keys and scan each one by port (faster than digitalRead)
  for (int i = 0; i < KEYCOUNT; i++) {
    // Switch based on the key number since the pins are not in port order.
    switch (i) {
      case 0:
        keyRead[i] = PIND & _BV(PD2);
        break;
      case 1:
        keyRead[i] = PIND & _BV(PD3);
        break;
      case 2:
        keyRead[i] = PIND & _BV(PD1);
        break;
      case 3:
        keyRead[i] = PIND & _BV(PD0);
        break;
      case 4:
        keyRead[i] = PIND & _BV(PD4);
        break;
      case 5:
        keyRead[i] = PINC & _BV(PC6);
        break;
      case 6:
        keyRead[i] = PIND & _BV(PD7);
        break;
      case 7:
        keyRead[i] = PINE & _BV(PE6);
        break;
      case 8:
        keyRead[i] = PINB & _BV(PB4);
        break;
      case 9:
        keyRead[i] = PINB & _BV(PB5);
        break;
      default:
        break;
    }
  }
}

void sendKeystroke(int i, bool down) {
  // If keycode at layer 1 is the function layer, toggle layer
  if (KEYMAP[0][i] == '^') {
    if (down) {
      layer = 1;
    } else {
      layer = 0;
    }
    return;
  }
  // Otherwise, switch based on char
  switch (KEYMAP[layer][i]) {
    // LED toggles on down
    case '$':
      if (down) {
        toggleLED();
      }
      break;
    // Null sends nothing
    case '\0':
      break;
    // Regular keys
    default:
      // Down sends the key at current layer
      // Also marks the key as pressed
      if (down) {
        Keyboard.press(KEYMAP[layer][i]);
        keyTracker[layer][i] = true;
      }
      // Releasing keys is more difficult.
      // To deal with layer changes, we go off of the keyTracker variable rather than the current layer key
      for (int j = 0; j <= 1; j++) {
        if (keyTracker[j][i] == true) {
          Keyboard.release(KEYMAP[j][i]);
          keyTracker[j][i] = false;
        }
      }
  }
}

void calculateKeys() {
  // Loop all keys
  for (int i = 0; i < KEYCOUNT; i++) {
    // If key is pressed
    if (keyRead[i]) {
      // Send keystroke if switching from unpressed
      if (keyStatus[i] == 0) {
        keyStatus[i] = 1;
        // Actual keystroke is handled by sendKeystroke()
        sendKeystroke(i, true);
      }
    }
    // Otherwise, if key is released,
    else {
      switch (keyStatus[i]) {
        // If current state is 1 (switching from pressed to non-pressed), begin debounce
        case 1:
          keyStatus[i] = 2;
          lastRelease[i] = millis();
          break;
        // Otherwise, if already debouncing, check timer
        case 2:
          if (millis() - lastRelease[i] > DEBOUNCETIME) {
            sendKeystroke(i, false);
          }
          break;
        // Otherwise, was unpressed and is unpressed. How boring
        default:
          break;
      }          
    }
  }
}

void loop() {
  scanKeys();
  // Now that key scanning is done, we switch to calculating keypresses.
  calculateKeys();
  calculateLED();
  delayMicroseconds(50);
}
