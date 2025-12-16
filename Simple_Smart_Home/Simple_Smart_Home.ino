/*
 * Projet Smart Home IoT - Version Multi-Utilisateurs (Partagée)
 * Tout le monde contrôle la même maison (Dossier "smart_home")
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ESP32Servo.h>

// --- VOS IDENTIFIANTS ---
#define WIFI_SSID       "MSI 4904"
#define WIFI_PASSWORD   "Mf54?813"

#define FIREBASE_URL    "https://smarthome-dbadb-default-rtdb.firebaseio.com/"
#define FIREBASE_SECRET "16VEgR6yMDETwAfo4SXUkGvVg06vL9CnFmKX3O4V"

// --- PINS ---
#define PIN_GAZ    34
#define PIN_SERVO  18
#define PIN_LED    23

Servo monServo;
unsigned long lastGasTime = 0;
unsigned long lastReadTime = 0;

void sendToFirebase(String method, String path, String data);
String getFromFirebase(String path);
String cleanPayload(String payload);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n--- SMART HOME PARTAGEE ---");

  pinMode(PIN_GAZ, INPUT);
  pinMode(PIN_LED, OUTPUT);
  monServo.attach(PIN_SERVO);
  monServo.write(0);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connexion WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\nWiFi Connecté!");
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.reconnect();
    return;
  }

  unsigned long now = millis();

  // --- A. ENVOYER GAZ (Chemin Commun) ---
  if (now - lastGasTime > 3000) {
    lastGasTime = now;
    int valGaz = analogRead(PIN_GAZ);
    String json = "{\"level\": " + String(valGaz) + "}";
    
    // CHANGEMENT: On écrit dans /smart_home/ au lieu de /users/{id}/
    sendToFirebase("PATCH", "smart_home/sensors/gas.json", json);
  }

  // --- B.  LIRE COMMANDES (Chemin Commun) ---
  if (now - lastReadTime > 1000) {
    lastReadTime = now;
    
    // 1. Servo
    String servoRaw = getFromFirebase("smart_home/doors/main_door/position.json");
    String servoClean = cleanPayload(servoRaw);
    if (servoClean != "null" && servoClean != "") {
      monServo.write(servoClean.toInt());
    }

    // 2. LED
    String ledRaw = getFromFirebase("smart_home/lights/main_led/state.json");
    String ledClean = cleanPayload(ledRaw);
    if (ledClean == "true" || ledClean == "1") {
      digitalWrite(PIN_LED, HIGH);
    } else {
      digitalWrite(PIN_LED, LOW);
    }
  }
}

// --- OUTILS ---

String cleanPayload(String payload) {
  payload.trim();
  if (payload.startsWith("\"")) payload.remove(0, 1);
  if (payload.endsWith("\"")) payload.remove(payload.length() - 1);
  return payload;
}

void sendToFirebase(String method, String path, String data) {
  HTTPClient http;
  WiFiClientSecure client;
  client.setInsecure();
  String url = String(FIREBASE_URL) + path;
  http.begin(client, url);
  
  int httpCode = 0;
  if (method == "PATCH") {
    httpCode = http.PATCH(data);
  } else {
    httpCode = http.PUT(data);
  }

  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK) {
       // Succès
    } else {
       Serial.printf("[Erreur Envoi] Code: %d\n", httpCode);
       Serial.println("Reponse: " + http.getString());
    }
  } else {
    Serial.printf("[Erreur Réseau] %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
}

String getFromFirebase(String path) {
  HTTPClient http;
  WiFiClientSecure client;
  client.setInsecure();
  String url = String(FIREBASE_URL) + path;
  http.begin(client, url);
  int httpCode = http.GET();
  String payload = "";
  if (httpCode > 0) {
     // Code 200 = OK
    if (httpCode == HTTP_CODE_OK) {
       // Succès
       payload = http.getString();
    } else {
       Serial.printf("[Erreur Firebase] Code: %d\n", httpCode);
       Serial.println("Reponse: " + http.getString());
    }
  } else {
    Serial.printf("[Erreur Réseau] %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
  return payload;
}
