/*
 * ESP32 Gas Sensor with Firebase Realtime Database
 * 
 * Hardware:
 * - ESP32 board
 * - MQ-2 or MQ-135 gas sensor
 * - Connections:
 *   - Sensor VCC -> 5V
 *   - Sensor GND -> GND
 *   - Sensor AO (Analog Out) -> GPIO34 (ADC1_CH6)
 * 
 * Features:
 * - Reads gas sensor values
 * - Publishes data to Firebase Realtime Database
 * - Color-coded status (safe/warning/danger)
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
#define GAS_SENSOR_PIN 34  // ADC1_CH6

// Firebase objects
FirebaseData firebaseData;
FirebaseAuth auth;
FirebaseConfig config;

// Sensor calibration
const int SAFE_THRESHOLD = 400;      // PPM
const int DANGER_THRESHOLD = 800;    // PPM
const int SENSOR_MIN = 0;
const int SENSOR_MAX = 4095;         // 12-bit ADC
const int PPM_MIN = 0;
const int PPM_MAX = 1000;

// Timing
unsigned long lastUpdate = 0;
const unsigned long UPDATE_INTERVAL = 2000;  // Update every 2 seconds

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 Gas Sensor Starting...");
  
  // Initialize sensor pin
  pinMode(GAS_SENSOR_PIN, INPUT);
  
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
  String basePath = "/users/" + String(USER_ID) + "/sensors/gas";
  Firebase.setInt(firebaseData, basePath + "/level", 0);
  Firebase.setString(firebaseData, basePath + "/status", "safe");
  
  Serial.println("Gas Sensor Ready!");
}

void loop() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastUpdate >= UPDATE_INTERVAL) {
    lastUpdate = currentTime;
    
    // Read sensor value
    int sensorValue = analogRead(GAS_SENSOR_PIN);
    
    // Convert to PPM (Parts Per Million)
    int gasLevel = map(sensorValue, SENSOR_MIN, SENSOR_MAX, PPM_MIN, PPM_MAX);
    
    // Determine status
    String gasStatus;
    if (gasLevel >= DANGER_THRESHOLD) {
      gasStatus = "danger";
    } else if (gasLevel >= SAFE_THRESHOLD) {
      gasStatus = "warning";
    } else {
      gasStatus = "safe";
    }
    
    // Print to Serial Monitor
    Serial.println("======================");
    Serial.print("Raw Sensor Value: ");
    Serial.println(sensorValue);
    Serial.print("Gas Level: ");
    Serial.print(gasLevel);
    Serial.println(" PPM");
    Serial.print("Status: ");
    Serial.println(gasStatus);
    
    // Update Firebase
    String basePath = "/users/" + String(USER_ID) + "/sensors/gas";
    
    if (Firebase.setInt(firebaseData, basePath + "/level", gasLevel)) {
      Serial.println("✓ Gas level updated in Firebase");
    } else {
      Serial.println("✗ Failed to update gas level");
      Serial.println("Reason: " + firebaseData.errorReason());
    }
    
    if (Firebase.setString(firebaseData, basePath + "/status", gasStatus)) {
      Serial.println("✓ Gas status updated in Firebase");
    } else {
      Serial.println("✗ Failed to update gas status");
      Serial.println("Reason: " + firebaseData.errorReason());
    }
    
    // Update timestamp
    Firebase.setTimestamp(firebaseData, basePath + "/lastUpdated");
  }
  
  delay(100);  // Small delay to prevent watchdog issues
}
