/**************************************************************
   README:
   --------
   ESP32 Power Button Simulator with State Verification
   ------------------------------------------------------------
   - This code monitors an input pin (TRIGGER_PIN) for a switch
     that toggles between OFF and ON.
   - It also monitors the motherboard power LED pin (POWER_LED_PIN)
     to determine whether the PC is ON or OFF.
   - Based on both inputs, it ensures a "button press" on the PC
     power pin (POWER_BTN_PIN) is only simulated when needed.

   Pin Usage (example):
   --------------------
   - TRIGGER_PIN (GPIO 32): input from your ON/OFF switch or signal
   - POWER_LED_PIN (GPIO 25): input from the motherboard's power LED pin
   - POWER_BTN_PIN (GPIO 4): output to motherboard power button header
   - LED_TRIGGER_STATE (GPIO 2): lights up to reflect stable trigger state
   - LED_PRESS_ACTIVE (GPIO 18): lights up during power-button press simulation

   Connections:
   ------------
   - ESP32 GND -> PC Power Supply / Motherboard GND
   - Motherboard power-button header pin (3.3V side) -> POWER_BTN_PIN (GPIO 4)
   - Motherboard power LED pin -> POWER_LED_PIN (GPIO 25)
   - Debug LEDs connected to LED_TRIGGER_STATE and LED_PRESS_ACTIVE.
**************************************************************/

// Pin definitions
const int TRIGGER_PIN       = 32; // GPIO for your ON/OFF input signal
const int POWER_LED_PIN     = 19; // GPIO to read PC power LED state
const int POWER_BTN_PIN     = 4;  // GPIO that drives the motherboard power button pin
const int LED_TRIGGER_STATE = 2;  // LED to show stable trigger state
const int LED_PRESS_ACTIVE  = 18; // LED to show when we're "pressing" the power button

// Debounce parameters
const unsigned long DEBOUNCE_MS = 50; // time (ms) to wait for a stable signal

// Variables for debounce logic
bool lastTriggerReading  = HIGH; // last raw reading from TRIGGER_PIN
bool stableTriggerState  = HIGH; // debounced "stable" trigger state
unsigned long lastChangeTime = 0;  // when we last saw the input change

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== ESP32 Power Button Simulator with State Verification ===");

  // Configure pins
  pinMode(TRIGGER_PIN, INPUT_PULLUP);    // External ON/OFF switch
  pinMode(POWER_LED_PIN, INPUT);  // PC power LED pin
  pinMode(POWER_BTN_PIN, INPUT);         // Initially high-impedance
  pinMode(LED_TRIGGER_STATE, OUTPUT);    // Debug LED for trigger state
  pinMode(LED_PRESS_ACTIVE, OUTPUT);     // Debug LED for button press

  // Initialize LEDs
  digitalWrite(LED_TRIGGER_STATE, LOW);
  digitalWrite(LED_PRESS_ACTIVE, LOW);

  // Initialize debounce logic
  lastTriggerReading = digitalRead(TRIGGER_PIN);
  stableTriggerState = lastTriggerReading;
  lastChangeTime = millis();

  // Update the trigger state LED
  digitalWrite(LED_TRIGGER_STATE, stableTriggerState ? HIGH : LOW);
}

void loop() {
  // Read the raw input from TRIGGER_PIN
  bool currentTriggerReading = digitalRead(TRIGGER_PIN);
  unsigned long now = millis();

  // Debounce logic: Detect stable changes on TRIGGER_PIN
  if (currentTriggerReading != lastTriggerReading) {
    lastChangeTime = now;
    lastTriggerReading = currentTriggerReading;
  }

  if ((now - lastChangeTime) > DEBOUNCE_MS && currentTriggerReading != stableTriggerState) {
    // Stable change detected
    stableTriggerState = currentTriggerReading;

    Serial.print("New stable trigger state: ");
    Serial.println(stableTriggerState ? "HIGH" : "LOW");

    // Update trigger state LED
    digitalWrite(LED_TRIGGER_STATE, stableTriggerState ? HIGH : LOW);

    // Check power LED state and decide whether to simulate a button press
    handleTriggerChange(stableTriggerState);
  }

  delay(10); // Reduce CPU usage
}

/**
 * Handle stable trigger changes based on the power LED state.
 * @param newTriggerState - The new stable state of TRIGGER_PIN.
 */
void handleTriggerChange(bool newTriggerState) {
  bool powerLEDState = digitalRead(POWER_LED_PIN); // Check if PC is ON (LED is active)

  if (newTriggerState == HIGH) {
    // TRIGGER_PIN went OFF -> ON
    if (!powerLEDState) {
      // Power LED is OFF (PC is OFF), simulate power ON
      simulatePowerButtonPress();
    } else {
      // Power LED is ON (PC is already ON), do nothing
      Serial.println("PC is already ON, no action needed.");
    }
  } else {
    // TRIGGER_PIN went ON -> OFF
    if (powerLEDState) {
      // Power LED is ON (PC is ON), simulate power OFF
      simulatePowerButtonPress();
    } else {
      // Power LED is OFF (PC is already OFF), do nothing
      Serial.println("PC is already OFF, no action needed.");
    }
  }
}

/**
 * Simulate a brief press of the motherboard's power button.
 */
void simulatePowerButtonPress() {
  Serial.println("Simulating power button press...");

  // Light up the press-active LED
  digitalWrite(LED_PRESS_ACTIVE, HIGH);

  // Drive pin LOW (short to ground)
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
