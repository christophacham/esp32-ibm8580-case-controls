/**************************************************************
   README:
   --------
   ESP32 Power Button Simulator with Debouncing + Debug LEDs
   ------------------------------------------------------------
   - This code monitors an input pin (TRIGGER_PIN) for a switch
     that toggles between OFF and ON.
   - On a stable change (OFF->ON or ON->OFF), it simulates a momentary
     press of the PC's power button by pulling the motherboard's
     power-button pin to ground for ~200 ms.
   - The code uses software debouncing to avoid multiple rapid presses
     from switch chatter.
   - It also lights up two debug LEDs to indicate the current trigger
     state and the "press" event in real time.

   WARNING:
   --------
   - This direct approach (shorting the motherboard's power pin to GND
     via an ESP32 GPIO) is not the most robust or electrically safe.
   - Ideally, you'd use a transistor, optocoupler, or relay to protect
     both the ESP32 and the motherboard.
   - Ensure:
     1) The motherboard power button line is indeed 3.3 V when idle.
     2) The ESP32 shares the same ground as the motherboard (PC PSU GND).
   - Use at your own risk!

   Pin Usage (example):
   --------------------
   - TRIGGER_PIN (GPIO 32):  input from your ON/OFF switch or signal
   - POWER_BTN_PIN (GPIO 4): output to motherboard power button header
   - LED_TRIGGER_STATE (GPIO 14): lights up to reflect stable trigger state
   - LED_PRESS_ACTIVE (GPIO 16): lights up during power-button press simulation

   Connections:
   ------------
   - ESP32 GND -> PC Power Supply / Motherboard GND
   - Motherboard power-button header pin (3.3V side) -> POWER_BTN_PIN (GPIO 4)
   - (If used) case power button -> same two header pins on motherboard
     (the ESP32 connection can be in parallel or replacing the button)
   - TRIGGER_PIN wired to your external switch or logic signal (with a
     pull-up or pull-down as needed)
   - Debug LEDs: connected from each LED pin to a resistor, then to 3.3V or GND
     (depending on your LED wiring preference).

   License:
   --------
   Feel free to modify and use for any purpose. 
**************************************************************/

// Pin definitions
const int TRIGGER_PIN       = 32; // GPIO for your OFF/ON input signal
const int POWER_BTN_PIN     = 4;  // GPIO that drives the motherboard power-button pin
const int LED_TRIGGER_STATE = 2; // LED to show stable trigger state
const int LED_PRESS_ACTIVE  = 18; // LED to show when we're "pressing" the power button

// Debounce parameters
const unsigned long DEBOUNCE_MS = 50; // time (ms) to wait for a stable signal

// Variables for debounce logic
bool lastReading        = HIGH; // the last raw reading from TRIGGER_PIN
bool stableTriggerState = HIGH; // the debounced "stable" state we trust
unsigned long lastChangeTime    = 0;  // when we last saw the input change

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== ESP32 Power Button Simulator (Debouncing + LEDs) ===");

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

      // Simulate a power-button press for any stable transition
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
