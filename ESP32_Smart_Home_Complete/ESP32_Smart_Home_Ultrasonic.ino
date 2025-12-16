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
 * - 1x HC-SR04 Ultrasonic (Détection de Présence) ⭐ NEW
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ESP32Servo.h>
#include <DHT.h>

// --- VOS IDENTIFIANTS ---
#define WIFI_SSID       "MSI 4904"
#define WIFI_PASSWORD   "Mf54?813"

#define FIREBASE_URL    "https://smarthome-dbadb-default-rtdb.firebaseio.com"
#define FIREBASE_SECRET "16VEgR6yMDETwAfo4SXUkGvVg06vL9CnFmKX3O4V"

// --- PINS ---
#define PIN_GAZ         34
#define PIN_SERVO_DOOR  18
#define PIN_SERVO_WIN   19
#define PIN_DHT         4
#define PIN_LED_LIVING  23
#define PIN_LED_KITCHEN 22
#define PIN_LED_BEDROOM 21
#define PIN_BUZZER      5
#define PIN_TRIG        13  // ⭐ NEW: Ultrasonic TRIG
#define PIN_ECHO        12  // ⭐ NEW: Ultrasonic ECHO

// --- CONFIGURATION ULTRASONIC ---
#define PRESENCE_THRESHOLD 50  // Distance en cm pour détecter une présence
#define COOLDOWN_TIME 10000    // 10 secondes entre 2 détections (évite spam)

// --- OBJETS ---
Servo doorServo;
Servo windowServo;
DHT dht(PIN_DHT, DHT11);

// --- VARIABLES DE TEMPS ---
unsigned long lastSensorTime = 0;
unsigned long lastReadTime = 0;
unsigned long lastPresenceTime = 0;  // ⭐ NEW: Cooldown pour présence
bool presenceDetected = false;       // ⭐ NEW: État de présence

// --- PROTOTYPES ---
void sendToFirebase(String method, String path, String data);
String getFromFirebase(String path);
String cleanPayload(String payload);
bool parseBool(String raw);
long getUltrasonicDistance();  // ⭐ NEW

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n--- SMART HOME PARTAGEE (MULTI-ROOM + ULTRASONIC) ---");

  // Init Capteurs/Actuateurs
  pinMode(PIN_GAZ, INPUT);
  pinMode(PIN_LED_LIVING, OUTPUT);
  pinMode(PIN_LED_KITCHEN, OUTPUT);
  pinMode(PIN_LED_BEDROOM, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);
  
  // ⭐ NEW: Ultrasonic
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  digitalWrite(PIN_TRIG, LOW);
  
  doorServo.attach(PIN_SERVO_DOOR);
  doorServo.write(90);
  
  windowServo.attach(PIN_SERVO_WIN);
  windowServo.write(90);

  dht.begin();

  // Connexion WiFi
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true); 
  delay(1000);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connexion WiFi à ");
  Serial.println(WIFI_SSID);
  
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
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi perdu! Tentative de reconnexion automatique...");
    WiFi.disconnect();
    WiFi.reconnect();
    delay(5000);
    return;
  }

  unsigned long now = millis();

  // --- A. DÉTECTION DE PRÉSENCE (Ultrasonic - Continu) --- ⭐ NEW
  long distance = getUltrasonicDistance();
  
  // ⭐ AFFICHAGE CONTINU DE LA DISTANCE (pour debug)
  if (distance > 0) {
    Serial.print("📏 Distance: ");
    Serial.print(distance);
    Serial.print(" cm");
    
    if (distance < PRESENCE_THRESHOLD) {
      Serial.println(" ⚠️ [ZONE DE DÉTECTION]");
    } else {
      Serial.println(" ✓ [Zone libre]");
    }
  } else {
    Serial.println("❌ Erreur lecture capteur (hors portée)");
  }
  
  // Détection: Quelqu'un s'approche (< 50cm)
  if (distance > 0 && distance < PRESENCE_THRESHOLD) {
    // Vérifier si on n'a pas déjà envoyé récemment (cooldown)
    if (!presenceDetected || (now - lastPresenceTime > COOLDOWN_TIME)) {
      Serial.println("");
      Serial.println("╔═══════════════════════════════════╗");
      Serial.println("║  🚨 ALERTE: PRÉSENCE DÉTECTÉE! 🚨 ║");
      Serial.println("╚═══════════════════════════════════╝");
      Serial.print("👤 Distance détectée: ");
      Serial.print(distance);
      Serial.println(" cm");
      Serial.println("📤 Envoi notification à Firebase...");
      
      // Envoyer à Firebase pour déclencher Face Recognition sur Raspberry Pi
      String presenceJson = "{\"detected\": true, \"distance\": " + String(distance) + ", \"timestamp\": " + String(now) + "}";
      sendToFirebase("PATCH", "/smart_home/sensors/presence.json", presenceJson);
      Serial.println("✅ Notification envoyée!");
      Serial.println("⏳ Cooldown de 10 secondes activé");
      Serial.println("");
      
      presenceDetected = true;
      lastPresenceTime = now;
    } else {
      // En cooldown, juste afficher
      Serial.print("⏸️  Cooldown actif (");
      Serial.print((COOLDOWN_TIME - (now - lastPresenceTime)) / 1000);
      Serial.println("s restantes)");
    }
  } else if (distance >= PRESENCE_THRESHOLD) {
    // Plus de présence - Réinitialiser
    if (presenceDetected) {
      Serial.println("");
      Serial.println("═══════════════════════════════════");
      Serial.println("✅ Zone dégagée - Fin de détection");
      Serial.println("═══════════════════════════════════");
      Serial.println("");
      
      String presenceJson = "{\"detected\": false, \"distance\": " + String(distance) + "}";
      sendToFirebase("PATCH", "/smart_home/sensors/presence.json", presenceJson);
      presenceDetected = false;
    }
  }
  
  // Petit délai pour ne pas spammer le Serial Monitor
  delay(500);

  // --- B. ENVOI CAPTEURS (Toutes les 3s) ---
  if (now - lastSensorTime > 3000) {
    lastSensorTime = now;
    
    // 1. Gaz
    int valGaz = analogRead(PIN_GAZ);
    
    if (valGaz > 2000) {
      digitalWrite(PIN_BUZZER, HIGH);
    } else {
      digitalWrite(PIN_BUZZER, LOW);
    }

    String gasJson = "{\"level\": " + String(valGaz) + "}";
    sendToFirebase("PATCH", "/smart_home/sensors/gas.json", gasJson);

    // 2. DHT11 (Temp/Hum)
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (!isnan(t) && !isnan(h)) {
      String climJson = "{\"temperature\": " + String(t) + ", \"humidity\": " + String(h) + "}";
      sendToFirebase("PATCH", "/climate.json", climJson);
    }
  }

  // --- C. LECTURE COMMANDES (Toutes les 1s) ---
  if (now - lastReadTime > 1000) {
    lastReadTime = now;
    
    // 1. Servo Porte
    String doorRaw = getFromFirebase("/smart_home/doors/main_door/position.json");
    String doorClean = cleanPayload(doorRaw);
    if (doorClean != "null" && doorClean != "") {
      doorServo.write(doorClean.toInt());
    }

    // 2. Servo Fenêtre (Salon)
    String winRaw = getFromFirebase("/rooms/living/window.json");
    bool winState = parseBool(winRaw);
    if (winState) windowServo.write(90);
    else windowServo.write(0);

    // 3. LED Salon
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

// --- ⭐ NEW: FONCTION ULTRASONIC ---
long getUltrasonicDistance() {
  // 1. Envoyer impulsion de 10µs sur TRIG
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  
  // 2. Lire la durée de l'impulsion ECHO
  long duration = pulseIn(PIN_ECHO, HIGH, 30000); // Timeout 30ms
  
  // 3. Calculer la distance en cm
  // Vitesse du son = 343 m/s = 0.034 cm/µs
  // Distance = (durée * vitesse) / 2
  long distance = duration * 0.034 / 2;
  
  // Filtrer les valeurs aberrantes
  if (distance < 2 || distance > 400) {
    return -1; // Hors portée
  }
  
  return distance;
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
