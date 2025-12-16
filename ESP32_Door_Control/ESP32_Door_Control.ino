/*
 * ESP32 Door Control (Servo Motor) with Firebase Realtime Database
 * 
 * Hardware:
 * - ESP32 board
 * - SG90 or MG996R servo motor
 * - External power supply (5-6V, 1-2A for larger servos)
 * - Connections:
 *   - Servo Signal -> GPIO18
 *   - Servo VCC -> External 5V (NOT ESP32 5V for large servos)
 *   - Servo GND -> Common GND (ESP32 + External Power)
 * 
 * Features:
 * - Listens to Firebase for door control commands
 * - Controls servo motor position (0-180 degrees)
 * - Reports current position back to Firebase
 */

#include <WiFi.h>
#include <FirebaseESP32.h>
#include <ESP32Servo.h>

// WiFi credentials
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// Firebase credentials
#define FIREBASE_HOST "YOUR_PROJECT_ID.firebaseio.com"
#define FIREBASE_AUTH "YOUR_DATABASE_SECRET"

// User ID - Get this from your Firebase Authentication
#define USER_ID "YOUR_USER_ID"

// Pin definitions
#define SERVO_PIN 18

// Firebase objects
FirebaseData firebaseData;
FirebaseData stream;
FirebaseAuth auth;
FirebaseConfig config;

// Servo object
Servo doorServo;

// Door state
int currentPosition = 0;
String currentStatus = "closed";

// Servo limits
const int SERVO_MIN = 0;
const int SERVO_MAX = 180;

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 Door Control Starting...");
  
  // Initialize servo
  doorServo.attach(SERVO_PIN);
  doorServo.write(0);  // Start at closed position
  
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
  String basePath = "/users/" + String(USER_ID) + "/doors/main_door";
  Firebase.setInt(firebaseData, basePath + "/position", 0);
  Firebase.setString(firebaseData, basePath + "/status", "closed");
  
  // Begin streaming changes from Firebase
  if (!Firebase.beginStream(stream, basePath + "/position")) {
    Serial.println("Failed to begin stream");
    Serial.println("Reason: " + stream.errorReason());
  }
  
  Serial.println("Door Control Ready!");
}

void loop() {
  // Check for stream updates
  if (Firebase.readStream(stream)) {
    if (stream.streamAvailable()) {
      if (stream.dataType() == "int") {
        int newPosition = stream.intData();
        
        // Constrain position to valid range
        newPosition = constrain(newPosition, SERVO_MIN, SERVO_MAX);
        
        if (newPosition != currentPosition) {
          Serial.println("======================");
          Serial.print("New position command: ");
          Serial.print(newPosition);
          Serial.println("°");
          
          // Move servo smoothly
          moveDoorSmoothly(currentPosition, newPosition);
          currentPosition = newPosition;
          
          // Update status based on position
          if (newPosition < 45) {
            currentStatus = "closed";
          } else if (newPosition > 135) {
            currentStatus = "open";
          } else {
            currentStatus = "partially_open";
          }
          
          Serial.print("Door Status: ");
          Serial.println(currentStatus);
          
          // Update status in Firebase
          String basePath = "/users/" + String(USER_ID) + "/doors/main_door";
          Firebase.setString(firebaseData, basePath + "/status", currentStatus);
          Firebase.setTimestamp(firebaseData, basePath + "/lastUpdated");
        }
      }
    }
  }
  
  delay(50);  // Small delay
}

// Smoothly move servo from current position to target position
void moveDoorSmoothly(int fromPos, int toPos) {
  int step = (fromPos < toPos) ? 1 : -1;
  int delayTime = 15;  // Adjust for speed (lower = faster)
  
  for (int pos = fromPos; pos != toPos; pos += step) {
    doorServo.write(pos);
    delay(delayTime);
  }
  
  // Ensure we reach the exact target position
  doorServo.write(toPos);
  Serial.println("✓ Servo moved to position");
}
