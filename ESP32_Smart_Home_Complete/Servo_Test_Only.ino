/*
 * TEST SIMPLE SERVO MOTEURS
 * Ce code teste uniquement les servos pour vérifier l'alimentation et le câblage.
 * 
 * CABLAGE:
 * - Servo Porte   -> PIN 18
 * - Servo Fenêtre -> PIN 19
 * - VCC Servo     -> 5V (VIN) ou Alim Externe (Pas 3.3V !)
 * - GND Servo     -> GND ESP32
 */

#include <ESP32Servo.h>

#define PIN_SERVO_DOOR  18
#define PIN_SERVO_WIN   19

Servo doorServo;
Servo windowServo;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n--- TEST SERVO MOTEURS (SWEEP) ---");

  // Configuration "Lourde" pour éviter les blocages
  doorServo.setPeriodHertz(50);
  doorServo.attach(PIN_SERVO_DOOR, 500, 2400);

  windowServo.setPeriodHertz(50);
  windowServo.attach(PIN_SERVO_WIN, 500, 2400); // 500-2400 est standard pour SG90/MG90S

  Serial.println("Initialisation à 10 degrés...");
  doorServo.write(10);
  windowServo.write(10);
  delay(2000);
}

void loop() {
  Serial.println("\n🔄 Ouverture Rapide (Sweep)...");
  
  // Plus rapide : 15ms au lieu de 50ms
  for (int pos = 10; pos <= 170; pos += 5) { 
    doorServo.write(pos);
    windowServo.write(pos);
    delay(15); 
  }
  
  Serial.println("✅ Ouvert ! Pause 1 sec.");
  delay(1000);

  Serial.println("🔄 Fermeture Rapide (Sweep)...");
  
  for (int pos = 170; pos >= 10; pos -= 5) { 
    doorServo.write(pos);
    windowServo.write(pos);
    delay(15);
  }
  
  Serial.println("✅ Fermé !");
  delay(1000);
  
  // Test "Instantanné" (Risqué pour l'alim mais test réaliste)
  Serial.println("⚡ Test Réaction Immédiate (Jump)...");
  doorServo.write(170);
  windowServo.write(170);
  delay(1000);
  doorServo.write(10);
  windowServo.write(10);
  delay(2000);
}
