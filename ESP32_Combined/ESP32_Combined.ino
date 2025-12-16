/*
 * ESP32 Smart Home Controller - Gas Sensor + Door Control + LED
 * 
 * Hardware:
 * - ESP32 board
 * - MQ-2 or MQ-135 gas sensor
 * - SG90 or MG996R servo motor
 * - LED (any color) with 220-330Ω resistor
 * - External power supply (5-6V, 1-2A for servo)
 * 
 * Connections:
 * - Gas Sensor AO -> GPIO34 (ADC1_CH6)
 * - Gas Sensor VCC -> 5V
 * - Gas Sensor GND -> GND
 * - Servo Signal -> GPIO18
 * - Servo VCC -> External 5V (use external power for MG996R)
 * - Servo GND -> Common GND (ESP32 + External Power)
 * - LED Anode (+) -> GPIO23 (via 220Ω resistor)
 * - LED Cathode (-) -> GND
 * 
 * Features:
 * - Real-time gas monitoring with Firebase sync
 * - Servo motor door control via Firebase commands
 * - LED control with PWM brightness (0-255)
 * - Multi-room support (easily expandable)
 */

#include <WiFi.h>
#include <FirebaseESP32.h>
#include <ESP32Servo.h>

// ==================== CONFIGURATION ====================

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
#define SERVO_PIN 18
#define LED_PIN 23         // PWM capable pin

// PWM Configuration for LED
#define LED_PWM_CHANNEL 0
#define LED_PWM_FREQ 5000
#define LED_PWM_RESOLUTION 8  // 8-bit (0-255)

// Gas sensor thresholds (PPM)
const int SAFE_THRESHOLD = 400;
const int DANGER_THRESHOLD = 800;

// Timing intervals
const unsigned long GAS_UPDATE_INTERVAL = 2000;  // 2 seconds
const unsigned long HEARTBEAT_INTERVAL = 5000;   // 5 seconds

// ==================== GLOBAL OBJECTS ====================

FirebaseData firebaseData;
FirebaseData streamData;
FirebaseData ledStreamData;
FirebaseAuth auth;
FirebaseConfig config;

Servo doorServo;

// State variables
int currentDoorPosition = 0;
String currentDoorStatus = "closed";
bool currentLEDState = false;
int currentBrightness = 255;
unsigned long lastGasUpdate = 0;
unsigned long lastHeartbeat = 0;

// ==================== SETUP ====================

void setup() {
  Serial.begin(115200);
  Serial.println("\n\n================================");
  Serial.println("ESP32 Smart Home Controller");
  Serial.println("Gas Sensor + Door Control");
  Serial.println("================================\n");
  
  // Initialize hardware
  pinMode(GAS_SENSOR_PIN, INPUT);
  doorServo.attach(SERVO_PIN);
  doorServo.write(0);  // Start closed
  
  // Initialize LED with PWM
  ledcSetup(LED_PWM_CHANNEL, LED_PWM_FREQ, LED_PWM_RESOLUTION);
  ledcAttachPin(LED_PIN, LED_PWM_CHANNEL);
  ledcWrite(LED_PWM_CHANNEL, 0);  // Start OFF
  
  // Connect to WiFi
  connectWiFi();
  
  // Initialize Firebase
  initFirebase();
  
  // Set initial values in Firebase
  initializeFirebaseData();
  
  // Start listening for door commands
  String doorPath = "/users/" + String(USER_ID) + "/doors/main_door/position";
  if (!Firebase.beginStream(streamData, doorPath)) {
    Serial.println("⚠ Failed to begin stream for door control");
    Serial.println("Reason: " + streamData.errorReason());
  } else {
    Serial.println("✓ Door control stream started");
  }
  
  // Start listening for LED state changes
  String ledStatePath = "/users/" + String(USER_ID) + "/lights/main_led/state";
  if (!Firebase.beginStream(ledStreamData, ledStatePath)) {
    Serial.println("⚠ Failed to begin stream for LED control");
    Serial.println("Reason: " + ledStreamData.errorReason());
  } else {
    Serial.println("✓ LED control stream started");
  }
  
  Serial.println("\n✅ System Ready!\n");
}

// ==================== MAIN LOOP ====================

void loop() {
  unsigned long currentTime = millis();
  
  // Update gas sensor readings
  if (currentTime - lastGasUpdate >= GAS_UPDATE_INTERVAL) {
    lastGasUpdate = currentTime;
    updateGasSensor();
  }
  
  // Listen for door control commands
  handleDoorControl();
  
  // Listen for LED control commands
  handleLEDControl();
  
  // Heartbeat to show system is alive
  if (currentTime - lastHeartbeat >= HEARTBEAT_INTERVAL) {
    lastHeartbeat = currentTime;
    Serial.println("♥ Heartbeat - System running...");
  }
  
  delay(50);
}

// ==================== WIFI FUNCTIONS ====================

void connectWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✓ WiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n✗ WiFi Connection Failed!");
    Serial.println("Please check credentials and restart");
  }
}

// ==================== FIREBASE FUNCTIONS ====================

void initFirebase() {
  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  
  Serial.println("✓ Firebase Connected!");
}

void initializeFirebaseData() {
  String gasPath = "/users/" + String(USER_ID) + "/sensors/gas";
  String doorPath = "/users/" + String(USER_ID) + "/doors/main_door";
  String ledPath = "/users/" + String(USER_ID) + "/lights/main_led";
  
  // Initialize gas sensor data
  Firebase.setInt(firebaseData, gasPath + "/level", 0);
  Firebase.setString(firebaseData, gasPath + "/status", "safe");
  
  // Initialize door data
  Firebase.setInt(firebaseData, doorPath + "/position", 0);
  Firebase.setString(firebaseData, doorPath + "/status", "closed");
  
  // Initialize LED data
  Firebase.setBool(firebaseData, ledPath + "/state", false);
  Firebase.setInt(firebaseData, ledPath + "/brightness", 255);
  
  Serial.println("✓ Firebase data initialized");
}

// ==================== GAS SENSOR FUNCTIONS ====================

void updateGasSensor() {
  // Read sensor
  int sensorValue = analogRead(GAS_SENSOR_PIN);
  int gasLevel = map(sensorValue, 0, 4095, 0, 1000);  // Convert to PPM
  
  // Determine status
  String gasStatus;
  if (gasLevel >= DANGER_THRESHOLD) {
    gasStatus = "danger";
  } else if (gasLevel >= SAFE_THRESHOLD) {
    gasStatus = "warning";
  } else {
    gasStatus = "safe";
  }
  
  // Print status
  Serial.println("--- Gas Sensor ---");
  Serial.print("Level: ");
  Serial.print(gasLevel);
  Serial.print(" PPM | Status: ");
  Serial.println(gasStatus);
  
  // Update Firebase
  String basePath = "/users/" + String(USER_ID) + "/sensors/gas";
  Firebase.setInt(firebaseData, basePath + "/level", gasLevel);
  Firebase.setString(firebaseData, basePath + "/status", gasStatus);
  Firebase.setTimestamp(firebaseData, basePath + "/lastUpdated");
}

// ==================== DOOR CONTROL FUNCTIONS ====================

void handleDoorControl() {
  if (!Firebase.readStream(streamData)) {
    return;
  }
  
  if (streamData.streamAvailable()) {
    if (streamData.dataType() == "int") {
      int newPosition = streamData.intData();
      newPosition = constrain(newPosition, 0, 180);
      
      if (newPosition != currentDoorPosition) {
        Serial.println("--- Door Control ---");
        Serial.print("Moving to: ");
        Serial.print(newPosition);
        Serial.println("°");
        
        moveDoorSmoothly(currentDoorPosition, newPosition);
        currentDoorPosition = newPosition;
        
        // Update status
        if (newPosition < 45) {
          currentDoorStatus = "closed";
        } else if (newPosition > 135) {
          currentDoorStatus = "open";
        } else {
          currentDoorStatus = "partially_open";
        }
        
        Serial.print("Status: ");
        Serial.println(currentDoorStatus);
        
        // Update Firebase
        String basePath = "/users/" + String(USER_ID) + "/doors/main_door";
        Firebase.setString(firebaseData, basePath + "/status", currentDoorStatus);
        Firebase.setTimestamp(firebaseData, basePath + "/lastUpdated");
      }
    }
  }
}

void moveDoorSmoothly(int fromPos, int toPos) {
  int step = (fromPos < toPos) ? 1 : -1;
  
  for (int pos = fromPos; pos != toPos; pos += step) {
    doorServo.write(pos);
    delay(15);  // Adjust for smoother/faster movement
  }
  
  doorServo.write(toPos);
}

// ==================== LED CONTROL FUNCTIONS ====================

void handleLEDControl() {
  if (!Firebase.readStream(ledStreamData)) {
    return;
  }
  
  if (ledStreamData.streamAvailable()) {
    // LED state changed - read the complete data
    String ledPath = "/users/" + String(USER_ID) + "/lights/main_led";
    
    if (Firebase.getBool(firebaseData, ledPath + "/state")) {
      bool newState = firebaseData.boolData();
      
      if (Firebase.getInt(firebaseData, ledPath + "/brightness")) {
        int newBrightness = firebaseData.intData();
        
        if (newState != currentLEDState || newBrightness != currentBrightness) {
          currentLEDState = newState;
          currentBrightness = newBrightness;
          
          Serial.println("--- LED Control ---");
          Serial.print("State: ");
          Serial.print(currentLEDState ? "ON" : "OFF");
          
          if (currentLEDState) {
            Serial.print(" | Brightness: ");
            Serial.print(currentBrightness);
            Serial.print(" (");
            Serial.print((currentBrightness * 100) / 255);
            Serial.println("%)");
            
            // Set LED brightness with PWM
            ledcWrite(LED_PWM_CHANNEL, currentBrightness);
          } else {
            Serial.println();
            // Turn LED off
            ledcWrite(LED_PWM_CHANNEL, 0);
          }
        }
      }
    }
  }
}

