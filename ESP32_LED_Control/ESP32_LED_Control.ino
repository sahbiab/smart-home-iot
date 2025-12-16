/*
 * ESP32 LED Control with Firebase Realtime Database
 * 
 * Hardware:
 * - ESP32 board
 * - LED (any color) with 220-330Ω resistor
 * - Connections:
 *   - LED Anode (+) -> GPIO23 (via 220Ω resistor)
 *   - LED Cathode (-) -> GND
 * 
 * Features:
 * - On/Off control via Firebase
 * - PWM brightness control (0-255)
 * - Real-time synchronization with Flutter app
 */

#include <WiFi.h>
#include <FirebaseESP32.h>

// WiFi credentials
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// Firebase credentials
#define FIREBASE_HOST "YOUR_PROJECT_ID.firebaseio.com"
#define FIREBASE_AUTH "YOUR_DATABASE_SECRET"

// User ID - Get this from your Firebase Authentication
#define USER_ID "YOUR_USER_ID"

// Pin definitions
#define LED_PIN 23  // PWM capable pin

// PWM Configuration
#define LED_PWM_CHANNEL 0
#define LED_PWM_FREQ 5000
#define LED_PWM_RESOLUTION 8  // 8-bit (0-255)

// Firebase objects
FirebaseData firebaseData;
FirebaseData stream;
FirebaseAuth auth;
FirebaseConfig config;

// LED state
bool currentState = false;
int currentBrightness = 255;

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 LED Control Starting...");
  
  // Initialize LED with PWM
  ledcSetup(LED_PWM_CHANNEL, LED_PWM_FREQ, LED_PWM_RESOLUTION);
  ledcAttachPin(LED_PIN, LED_PWM_CHANNEL);
  ledcWrite(LED_PWM_CHANNEL, 0);  // Start OFF
  
  // Connect to WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  
  // Configure Firebase
  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  
  // Initialize Firebase
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  
  Serial.println("Firebase Connected!");
  
  // Set initial values
  String basePath = "/users/" + String(USER_ID) + "/lights/main_led";
  Firebase.setBool(firebaseData, basePath + "/state", false);
  Firebase.setInt(firebaseData, basePath + "/brightness", 255);
  
  // Begin streaming changes from Firebase
  if (!Firebase.beginStream(stream, basePath + "/state")) {
    Serial.println("Failed to begin stream");
    Serial.println("Reason: " + stream.errorReason());
  }
  
  Serial.println("LED Control Ready!");
}

void loop() {
  // Check for stream updates
  if (Firebase.readStream(stream)) {
    if (stream.streamAvailable()) {
      // State changed - read complete LED data
      String basePath = "/users/" + String(USER_ID) + "/lights/main_led";
      
      if (Firebase.getBool(firebaseData, basePath + "/state")) {
        bool newState = firebaseData.boolData();
        
        if (Firebase.getInt(firebaseData, basePath + "/brightness")) {
          int newBrightness = firebaseData.intData();
          
          if (newState != currentState || newBrightness != currentBrightness) {
            currentState = newState;
            currentBrightness = newBrightness;
            
            Serial.println("======================");
            Serial.print("LED State: ");
            Serial.print(currentState ? "ON" : "OFF");
            
            if (currentState) {
              Serial.print(" | Brightness: ");
              Serial.print(currentBrightness);
              Serial.print(" (");
              Serial.print((currentBrightness * 100) / 255);
              Serial.println("%)");
              
              // Set LED brightness with PWM
              ledcWrite(LED_PWM_CHANNEL, currentBrightness);
              Serial.println("✓ LED turned ON");
            } else {
              Serial.println();
              // Turn LED off
              ledcWrite(LED_PWM_CHANNEL, 0);
              Serial.println("✓ LED turned OFF");
            }
          }
        }
      }
    }
  }
  
  delay(50);  // Small delay
}
