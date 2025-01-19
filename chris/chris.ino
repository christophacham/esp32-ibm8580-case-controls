/**************************************************************
   README:
   --------
   ESP32 Power Button Simulator with Debouncing + Debug LEDs
   + Rapid Switching (Hard Shutdown)
   ------------------------------------------------------------

   CHANGES:
   - In addition to the normal "momentary press" on stable transitions,
     we now watch for rapid toggles (e.g., OFF->ON->OFF->ON, etc.)
     within a short time. If the number of transitions in that short
     window is between 3 and 6 (inclusive), we do a "hard shutdown"
     by holding the power button pin LOW for ~4 seconds.

   USAGE:
   - Adjust the time window, thresholds, and hold duration to fit your needs.

**************************************************************/

// Pin definitions
const int TRIGGER_PIN       = 32; // GPIO for your OFF/ON input signal
const int POWER_BTN_PIN     = 4;  // GPIO that drives the motherboard power-button pin
const int LED_TRIGGER_STATE = 2;  // LED to show stable trigger state
const int LED_PRESS_ACTIVE  = 18; // LED to show when we're "pressing" the power button

// Debounce parameters
const unsigned long DEBOUNCE_MS = 50; // time (ms) to wait for a stable signal

// Variables for debounce logic
bool lastReading        = HIGH;  // the last raw reading from TRIGGER_PIN
bool stableTriggerState = HIGH;  // the debounced "stable" state we trust
unsigned long lastChangeTime    = 0;  // when we last saw the input change

// --- NEW: Variables for detecting rapid state changes ---
const unsigned long RAPID_CHANGE_WINDOW_MS = 2000; // 2-second window
const int MIN_RAPID_CHANGES_TRIGGER = 3;           // minimal # of toggles to trigger hard shutdown
const int MAX_RAPID_CHANGES_TRIGGER = 6;           // maximum # of toggles for hard shutdown
int rapidChangeCount               = 0;            // how many toggles in the current window
unsigned long firstChangeInWindow  = 0;            // timestamp of the first change in the current window

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== ESP32 Power Button Simulator (Debouncing + LEDs + Rapid Switching) ===");

  // 1) Configure the trigger pin:
  //    Use INPUT_PULLUP if your switch pulls the pin to GND when pressed.
  //    Or just INPUT if you have an external pull-down resistor.
  pinMode(TRIGGER_PIN, INPUT_PULLUP);

  // 2) Initialize the power button pin in high-impedance (INPUT) mode.
  pinMode(POWER_BTN_PIN, INPUT);

  // 3) Set up debug LEDs as outputs, start them OFF.
  pinMode(LED_TRIGGER_STATE, OUTPUT);
  pinMode(LED_PRESS_ACTIVE, OUTPUT);
  digitalWrite(LED_TRIGGER_STATE, LOW);
  digitalWrite(LED_PRESS_ACTIVE, LOW);

  // 4) Initialize the debounce state variables.
  lastReading        = digitalRead(TRIGGER_PIN);
  stableTriggerState = lastReading;
  lastChangeTime     = millis();

  // Set LED_TRIGGER_STATE according to the initial stable state.
  digitalWrite(LED_TRIGGER_STATE, stableTriggerState ? HIGH : LOW);
}

void loop() {
  // Read the raw input from TRIGGER_PIN
  bool currentReading = digitalRead(TRIGGER_PIN);
  unsigned long now   = millis();

  // 1. Check if the raw reading changed from the last reading
  if (currentReading != lastReading) {
    // Update the last change time
    lastChangeTime = now;
    // Update lastReading so we can detect further changes
    lastReading = currentReading;
  }

  // 2. If enough time has passed without another change,
  //    the reading is stable.
  if ((now - lastChangeTime) > DEBOUNCE_MS) {
    // If the stable state is different from what we had:
    if (currentReading != stableTriggerState) {
      // We have a new stable state
      stableTriggerState = currentReading;

      Serial.print("New stable trigger state: ");
      Serial.println(stableTriggerState ? "HIGH" : "LOW");

      // Update LED_TRIGGER_STATE to reflect the new stable state
      digitalWrite(LED_TRIGGER_STATE, stableTriggerState ? HIGH : LOW);

      // --- NEW: Track the rapid changes for possible hard shutdown ---
      trackRapidChanges();

      // Perform the normal short press
      simulatePowerButtonPress();
    }
  }

  // (Optional) small delay to reduce CPU usage
  delay(10);
}

/**
 * Simulates a brief "press" of the motherboard's power button.
 *  1) Drive POWER_BTN_PIN LOW (short to ground),
 *  2) Wait ~200 ms,
 *  3) Return it to high-impedance (INPUT).
 * Also lights up LED_PRESS_ACTIVE while "pressing."
 */
void simulatePowerButtonPress() {
  Serial.println("Simulating power button press...");
  // Light up the press-active LED
  digitalWrite(LED_PRESS_ACTIVE, HIGH);

  // Drive pin LOW
  pinMode(POWER_BTN_PIN, OUTPUT);
  digitalWrite(POWER_BTN_PIN, LOW);

  // Wait 200 ms
  delay(200);

  // Release the "button" by returning to INPUT
  pinMode(POWER_BTN_PIN, INPUT);

  // Turn off the press-active LED
  digitalWrite(LED_PRESS_ACTIVE, LOW);
  Serial.println("Released power button.");
}

/**
 * NEW: Track rapid toggles and trigger a "hard shutdown" if they occur
 *      within the specified threshold and time window.
 */
void trackRapidChanges() {
  unsigned long now = millis();

  // If this is the first toggle in a while, reset the window
  if ((now - firstChangeInWindow) > RAPID_CHANGE_WINDOW_MS) {
    // Start a new window
    rapidChangeCount      = 1;
    firstChangeInWindow   = now;
  } else {
    // We are within the window, increment the count
    rapidChangeCount++;
  }

  Serial.print("Rapid changes so far: ");
  Serial.println(rapidChangeCount);

  // Check if we are within the threshold for a hard shutdown
  if (rapidChangeCount >= MIN_RAPID_CHANGES_TRIGGER && rapidChangeCount <= MAX_RAPID_CHANGES_TRIGGER) {
    Serial.println("TRIGGERING HARD SHUTDOWN (rapid toggles detected)...");
    simulateHardShutdown();
    // Reset the counter and window to avoid repeated triggers
    rapidChangeCount = 0;
  }
}

/**
 * NEW: Simulates a "hard shutdown" by holding the motherboard power
 *      button pin LOW for ~4 seconds. (Adjust as desired.)
 */
void simulateHardShutdown() {
  // Turn on LED_PRESS_ACTIVE as a visual indicator
  digitalWrite(LED_PRESS_ACTIVE, HIGH);

  // Drive pin LOW
  pinMode(POWER_BTN_PIN, OUTPUT);
  digitalWrite(POWER_BTN_PIN, LOW);

  // Wait ~4 seconds to force a hard power-off
  unsigned long shutdownStart = millis();
  while (millis() - shutdownStart < 4000) {
    // You could blink the LED_PRESS_ACTIVE or do other feedback here
    // but be aware of blocking calls
    delay(50);
  }

  // Release the "button" by returning to INPUT
  pinMode(POWER_BTN_PIN, INPUT);

  // Turn off the press-active LED
  digitalWrite(LED_PRESS_ACTIVE, LOW);
  Serial.println("Hard shutdown complete.");
}
