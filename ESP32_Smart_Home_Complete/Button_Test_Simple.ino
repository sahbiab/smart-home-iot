/*
 * Simple Button Test - GPIO 15
 * Use this to verify your button wiring is correct
 * 
 * WIRING:
 * - Red wire:  ESP32 GPIO 15  →  Button Pin (one side)
 * - Blue wire: ESP32 GND      →  Button Pin (diagonal/opposite side)
 * 
 * IMPORTANT: Connect to DIAGONAL pins on the button!
 */

#define PIN_BUTTON 15

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  
  Serial.println("\n\n=================================");
  Serial.println("   BUTTON TEST - GPIO 15");
  Serial.println("=================================");
  Serial.println("Press the button to test...\n");
}

void loop() {
  int state = digitalRead(PIN_BUTTON);
  
  // Print state every 500ms
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 500) {
    Serial.print("Button: ");
    
    if (state == LOW) {
      Serial.println("✓✓✓ PRESSED (LOW) ✓✓✓");
    } else {
      Serial.println("--- NOT PRESSED (HIGH) ---");
    }
    
    lastPrint = millis();
  }
  
  delay(10);
}
