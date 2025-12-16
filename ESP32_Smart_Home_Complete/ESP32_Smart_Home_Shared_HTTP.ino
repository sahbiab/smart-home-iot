/*
 * Projet Smart Home IoT - Version Multi-Utilisateurs (Partagée)
 * Tout le monde contrôle la même maison (Dossier "smart_home")
 * 
 * MODIFICATIONS:
 * - 3x LEDs (Salon, Cuisine, Chambre)
 * - 1x Servo Fenêtre (Salon)
 * - 1x DHT11 (Climat)
 * - 1x Servo Porte (Entrée)
 * - 1x Capteur Gaz
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ESP32Servo.h>
#include <DHT.h>

// --- VOS IDENTIFIANTS ---
#define WIFI_SSID       "MSI 4904"
#define WIFI_PASSWORD   "Mf54?813"

#define FIREBASE_URL    "https://smarthome-dbadb-default-rtdb.firebaseio.com" // Removed trailing slash
#define FIREBASE_SECRET "16VEgR6yMDETwAfo4SXUkGvVg06vL9CnFmKX3O4V"

// --- PINS ---
#define PIN_GAZ         34
#define PIN_SERVO_DOOR  18
#define PIN_SERVO_WIN   19
#define PIN_DHT         4
#define PIN_LED_LIVING  23
#define PIN_LED_KITCHEN 22
#define PIN_LED_BEDROOM 21
#define PIN_BUZZER      5  // Nouveau Buzzer

// --- OBJETS ---
Servo doorServo;
Servo windowServo;
DHT dht(PIN_DHT, DHT11);

// --- VARIABLES DE TEMPS ---
unsigned long lastSensorTime = 0;
unsigned long lastReadTime = 0;

// --- PROTOTYPES ---
void sendToFirebase(String method, String path, String data);
String getFromFirebase(String path);
String cleanPayload(String payload);
bool parseBool(String raw);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n--- SMART HOME PARTAGEE (MULTI-ROOM) ---");

  // Init Capteurs/Actuateurs
  pinMode(PIN_GAZ, INPUT);
  pinMode(PIN_LED_LIVING, OUTPUT);
  pinMode(PIN_LED_KITCHEN, OUTPUT);
  pinMode(PIN_LED_BEDROOM, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);
  
  doorServo.attach(PIN_SERVO_DOOR);
  doorServo.write(90); // Initialisation à 90°
  
  windowServo.attach(PIN_SERVO_WIN);
  windowServo.write(90); // Initialisation à 90°

  dht.begin();

  // Connexion WiFi
  WiFi.mode(WIFI_STA);
  // Nettoyage préalable
  WiFi.disconnect(true); 
  delay(1000);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connexion WiFi à ");
  Serial.println(WIFI_SSID);
  
  // Boucle de connexion bloquante (C'est plus sûr ici)
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connecté!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  Serial.println("----------------------------------");
}

void loop() {
  // Simple vérification de connexion
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi perdu! Tentative de reconnexion automatique...");
    // Pas de disconnect/reconnect agressif, on laisse l'ESP gérer ou on fait un simple begin si nécessaire
    WiFi.disconnect();
    WiFi.reconnect();
    delay(5000); // Pause longue pour laisser le temps
    return;
  }

  unsigned long now = millis();

  // --- A. ENVOI CAPTEURS (Toutes les 3s) ---
  if (now - lastSensorTime > 3000) {
    lastSensorTime = now;
    
    // 1. Gaz
    int valGaz = analogRead(PIN_GAZ);
    
    // ALARME BUZZER (> 2000 = Danger)
    if (valGaz > 2000) {
      digitalWrite(PIN_BUZZER, HIGH);
    } else {
      digitalWrite(PIN_BUZZER, LOW);
    }

    String gasJson = "{\"level\": " + String(valGaz) + "}";
    // Note: App reads 'smart_home/sensors/gas'
    sendToFirebase("PATCH", "/smart_home/sensors/gas.json", gasJson);

    // 2. DHT11 (Temp/Hum)
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (!isnan(t) && !isnan(h)) {
      String climJson = "{\"temperature\": " + String(t) + ", \"humidity\": " + String(h) + "}";
      // Note: App reads 'climate' (ROOT)
      sendToFirebase("PATCH", "/climate.json", climJson);
    }
  }

  // --- B.  LECTURE COMMANDES (Toutes les 1s) ---
  if (now - lastReadTime > 1000) {
    lastReadTime = now;
    
    // 1. Servo Porte
    // Note: App writes to 'smart_home/doors/main_door'
    String doorRaw = getFromFirebase("/smart_home/doors/main_door/position.json");
    String doorClean = cleanPayload(doorRaw);
    if (doorClean != "null" && doorClean != "") {
      doorServo.write(doorClean.toInt());
    }

    // 2. Servo Fenêtre (Salon)
    // Note: App writes to 'rooms/living/window' (ROOT)
    String winRaw = getFromFirebase("/rooms/living/window.json");
    bool winState = parseBool(winRaw);
    if (winState) windowServo.write(90); // Ouvert
    else windowServo.write(0);           // Fermé

    // 3. LED Salon
    // Note: App writes to 'rooms/living/light' (ROOT)
    String ledLivRaw = getFromFirebase("/rooms/living/light.json");
    digitalWrite(PIN_LED_LIVING, parseBool(ledLivRaw) ? HIGH : LOW);

    // 4. LED Cuisine
    String ledKitRaw = getFromFirebase("/rooms/kitchen/light.json");
    digitalWrite(PIN_LED_KITCHEN, parseBool(ledKitRaw) ? HIGH : LOW);

    // 5. LED Chambre
    String ledBedRaw = getFromFirebase("/rooms/bedroom/light.json");
    digitalWrite(PIN_LED_BEDROOM, parseBool(ledBedRaw) ? HIGH : LOW);
  }
}

// --- OUTILS ---

String cleanPayload(String payload) {
  payload.trim();
  if (payload.startsWith("\"")) payload.remove(0, 1);
  if (payload.endsWith("\"")) payload.remove(payload.length() - 1);
  return payload;
}

bool parseBool(String raw) {
  String clean = cleanPayload(raw);
  return (clean == "true" || clean == "1");
}

void sendToFirebase(String method, String path, String data) {
  HTTPClient http;
  WiFiClientSecure client;
  client.setInsecure();
  String url = String(FIREBASE_URL) + path + "?auth=" + String(FIREBASE_SECRET);
  http.begin(client, url);
  
  int httpCode = 0;
  if (method == "PATCH") {
    httpCode = http.PATCH(data);
  } else {
    httpCode = http.PUT(data);
  }

  if (httpCode > 0) {
    if (httpCode != HTTP_CODE_OK) {
       Serial.printf("[Erreur Envoi] Code: %d\n", httpCode);
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
  String url = String(FIREBASE_URL) + path + "?auth=" + String(FIREBASE_SECRET);
  http.begin(client, url);
  
  int httpCode = http.GET();
  String payload = "";
  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK) {
       payload = http.getString();
    }
  }
  http.end();
  return payload;
}
