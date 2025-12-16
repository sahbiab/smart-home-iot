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
 * - 1x Bouton Poussoir (Push Button) ⭐ NEW
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
#define PIN_BUTTON      15  // ⭐ NEW: Bouton Poussoir

// --- CONFIGURATION BOUTON ---
#define DEBOUNCE_DELAY  50   // 50ms debounce
#define NOTIFICATION_COOLDOWN 5000  // 5 secondes entre notifications

// --- OBJETS ---
Servo doorServo;
Servo windowServo;
DHT dht(PIN_DHT, DHT11);

// --- VARIABLES DE TEMPS ---
unsigned long lastSensorTime = 0;
unsigned long lastReadTime = 0;
unsigned long lastDebounceTime = 0;  // ⭐ NEW: Pour debounce
unsigned long lastNotificationTime = 0;  // ⭐ NEW: Cooldown notification
float lastTemp = 0.0; // ⭐ NEW: Pour automation fenêtre

// --- VARIABLES BOUTON ---
bool lastButtonState = HIGH;  // ⭐ NEW: État précédent (HIGH = non pressé avec pullup)
bool buttonState = HIGH;      // ⭐ NEW: État actuel
bool lastSentButtonState = HIGH;  // ⭐ NEW: Dernier état envoyé à Firebase

// --- PROTOTYPES ---
void sendToFirebase(String method, String path, String data);
String getFromFirebase(String path);
String cleanPayload(String payload);
bool parseBool(String raw);
void checkButtonAndNotify();  // ⭐ NEW

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n--- SMART HOME PARTAGEE (MULTI-ROOM + BUTTON) ---");

  // Init Capteurs/Actuateurs
  pinMode(PIN_GAZ, INPUT);
  pinMode(PIN_LED_LIVING, OUTPUT);
  pinMode(PIN_LED_KITCHEN, OUTPUT);
  pinMode(PIN_LED_BEDROOM, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_BUTTON, INPUT_PULLUP);  // ⭐ NEW: Bouton avec résistance pullup interne
  digitalWrite(PIN_BUZZER, LOW);
  
  doorServo.attach(PIN_SERVO_DOOR);
  doorServo.write(0);  // ⭐ Position fermée au démarrage
  
  windowServo.attach(PIN_SERVO_WIN);
  windowServo.write(0);  // ⭐ Position fermée au démarrage

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
  
  // ⭐ Initialiser l'état du bouton dans Firebase
  Serial.println("📤 Initialisation de l'état du bouton dans Firebase...");
  String initJson = "{\"state\": false}"; // Simplifié: plus de status
  sendToFirebase("PATCH", "/smart_home/sensors/button.json", initJson);
  Serial.println("✅ Bouton initialisé");
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

  // --- A. LECTURE BOUTON AVEC DEBOUNCE --- ⭐ NEW
  int reading = digitalRead(PIN_BUTTON);
  
  // Si l'état a changé, réinitialiser le timer de debounce
  if (reading != lastButtonState) {
    lastDebounceTime = now;
  }
  
  // Si le temps de debounce est écoulé
  if ((now - lastDebounceTime) > DEBOUNCE_DELAY) {
    // Si l'état a vraiment changé
    if (reading != buttonState) {
      buttonState = reading;
      
      // Bouton pressé (LOW car INPUT_PULLUP)
      if (buttonState == LOW) {
        Serial.println("\n╔════════════════════════════════╗");
        Serial.println("║  🔘 BOUTON PRESSÉ!             ║");
        Serial.println("╚════════════════════════════════╝");
        
        // Envoyer l'état à Firebase
        String buttonJson = "{\"state\": true}";
        sendToFirebase("PATCH", "/smart_home/sensors/button.json", buttonJson);
        lastSentButtonState = LOW;
        
        // Vérifier si notification nécessaire
        checkButtonAndNotify();
        
      } else if (lastSentButtonState == LOW) {
        // Bouton relâché
        Serial.println("🔘 Bouton relâché");
        String buttonJson = "{\"state\": false}";
        sendToFirebase("PATCH", "/smart_home/sensors/button.json", buttonJson);
        lastSentButtonState = HIGH;
      }
    }
  }
  
  lastButtonState = reading;

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
      
      // ⭐ AUTOMATION: Si Temp > 30°C (et qu'on vient de dépasser le seuil), ouvrir fenêtre à 60°
      if (t > 30.0 && lastTemp <= 30.0) {
         Serial.println("🔥 CHALEUR DÉTECTÉE (>30°C)! Ouverture automatique fenêtre à 60°.");
         // On force la valeur à 60 (entier) dans Firebase. La boucle principale le lira ensuite.
         sendToFirebase("PUT", "/rooms/living/window.json", "60");
      }
      lastTemp = t;
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

    // 2. Servo Fenêtre (Salon) - Position control (0-180°)
    String winRaw = getFromFirebase("/rooms/living/window.json");
    String winClean = cleanPayload(winRaw);
    
    if (winClean != "null" && winClean != "") {
      int targetPos = 0;
      
      // Support rétrocompatible pour true/false
      if (winClean == "true" || winClean == "True") {
        targetPos = 60; // ⭐ MODIF: true = 60° (Demande Utilisateur)
      } else if (winClean == "false" || winClean == "False") {
        targetPos = 0;
      } else {
        targetPos = winClean.toInt(); // Utiliser la valeur en degrés (0-180)
      }
      
      // Sécurité
      if (targetPos < 0) targetPos = 0;
      if (targetPos > 180) targetPos = 180;
      
      windowServo.write(targetPos);
      // Serial.printf("🪟 Fenêtre: %d°\n", targetPos); // Uncomment for debug
    }

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

// --- ⭐ NEW: FONCTION VERIFICATION ET NOTIFICATION BOUTON ---
// --- ⭐ NEW: FONCTION VERIFICATION ET NOTIFICATION BOUTON ---
void checkButtonAndNotify() {
  // Vérifier le cooldown
  unsigned long now = millis();
  if (now - lastNotificationTime < NOTIFICATION_COOLDOWN) {
    Serial.println("⏸️  Cooldown actif - Notification ignorée");
    return;
  }
  
  // ⭐ SIMPLIFICATION MAJEURE: ON ENVOIE TOUJOURS
  // On ne vérifie plus le "status", car on veut que ça sonne à CHAQUE fois !
  
  Serial.println("\n╔══════════════════════════════════════╗");
  Serial.println("║  🔔 DING DONG ! SONNETTE ACTIVÉE !  ║");
  Serial.println("╚══════════════════════════════════════╝");
  Serial.println("📤 Envoi de notification à Firebase...");
  
  // Créer une notification dans /smart_home/notifications
  // Utiliser micros() pour s'assurer d'un ID unique même si spam rapide
  String timestamp = String(millis());
  
  String notifJson = "{\"type\": \"unknown_button\", \"message\": \"Quelqu\'un a sonné !\", \"timestamp\": " + timestamp + ", \"read\": false}";
  
  String notifPath = "/smart_home/notifications/btn_" + timestamp + ".json";
  sendToFirebase("PUT", notifPath, notifJson);
  
  Serial.println("✅ Notification envoyée!");
  Serial.println("⏳ Cooldown de 5 secondes activé");
  Serial.println("");
  
  lastNotificationTime = now;
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
