#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ESP32Servo.h>
#include <DHT.h>

#define WIFI_SSID       "MSI 4904"
#define WIFI_PASSWORD   "Mf54?813"

#define FIREBASE_URL    "https://smarthome-dbadb-default-rtdb.firebaseio.com"
#define FIREBASE_SECRET "16VEgR6yMDETwAfo4SXUkGvVg06vL9CnFmKX3O4V"

#define PIN_GAZ         34
#define PIN_SERVO_DOOR  18
#define PIN_SERVO_WIN   19
#define PIN_DHT         4
#define PIN_LED_LIVING  23
#define PIN_LED_KITCHEN 22
#define PIN_LED_BEDROOM 21
#define PIN_BUZZER      5
#define PIN_BUTTON      15

#define DEBOUNCE_DELAY  50
#define NOTIFICATION_COOLDOWN 5000

Servo doorServo;
Servo windowServo;
DHT dht(PIN_DHT, DHT11);

unsigned long lastSensorTime = 0;
unsigned long lastReadTime = 0;
unsigned long lastDebounceTime = 0;
unsigned long lastNotificationTime = 0;
float lastTemp = 0.0;

bool lastButtonState = HIGH;
bool buttonState = HIGH;
bool lastSentButtonState = HIGH;

void sendToFirebase(String method, String path, String data);
String getFromFirebase(String path);
String cleanPayload(String payload);
bool parseBool(String raw);
void checkButtonAndNotify();
unsigned long lastAutoWindowTime = 0;

void setup() {
  delay(1000);

  pinMode(PIN_GAZ, INPUT);
  pinMode(PIN_LED_LIVING, OUTPUT);
  pinMode(PIN_LED_KITCHEN, OUTPUT);
  pinMode(PIN_LED_BEDROOM, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  digitalWrite(PIN_BUZZER, LOW);
  
  doorServo.setPeriodHertz(50);
  doorServo.attach(PIN_SERVO_DOOR, 500, 2400);
  doorServo.write(10);  
  
  windowServo.setPeriodHertz(50);
  windowServo.attach(PIN_SERVO_WIN, 500, 2400);
  windowServo.write(10);

  dht.begin();

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true); 
  delay(1000);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  WiFi.setSleep(false); 
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
  }

  String initJson = "{\"state\": false}"; 
  sendToFirebase("PATCH", "/smart_home/sensors/button.json", initJson);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.disconnect();
    WiFi.reconnect();
    delay(5000);
    return;
  }

  unsigned long now = millis();

  int reading = digitalRead(PIN_BUTTON);
  
  if (reading != lastButtonState) {
    lastDebounceTime = now;
  }
  
  if ((now - lastDebounceTime) > DEBOUNCE_DELAY) {
    if (reading != buttonState) {
      buttonState = reading;
      
      if (buttonState == LOW) {
        String buttonJson = "{\"state\": true}";
        sendToFirebase("PATCH", "/smart_home/sensors/button.json", buttonJson);
        lastSentButtonState = LOW;
        
        checkButtonAndNotify();
        
        String releaseJson = "{\"state\": false}";
        sendToFirebase("PATCH", "/smart_home/sensors/button.json", releaseJson);
        lastSentButtonState = HIGH;
      }
    }
  }
  
  lastButtonState = reading;

  if (now - lastSensorTime > 3000) {
    lastSensorTime = now;
    
    int valGaz = analogRead(PIN_GAZ);
    
    if (valGaz > 2000) {
      digitalWrite(PIN_BUZZER, HIGH);
    } else {
      digitalWrite(PIN_BUZZER, LOW);
    }

    String gasJson = "{\"level\": " + String(valGaz) + "}";
    sendToFirebase("PATCH", "/smart_home/sensors/gas.json", gasJson);

    float t = dht.readTemperature();
    float h = dht.readHumidity();
    
    if (!isnan(t) && !isnan(h)) {
      String climJson = "{\"temperature\": " + String(t) + ", \"humidity\": " + String(h) + "}";
      sendToFirebase("PATCH", "/climate.json", climJson);
    
      if (t > 30.0) {
         if (now - lastAutoWindowTime > 5000) { 
             String winStatus = getFromFirebase("/rooms/living/window.json");
             int currentPos = cleanPayload(winStatus).toInt();
             
             if (currentPos < 10) {
                 sendToFirebase("PUT", "/rooms/living/window.json", "60");
             }
             lastAutoWindowTime = now;
         }
      } 
      else if (t < 29.0) {
         if (now - lastAutoWindowTime > 5000) { 
             sendToFirebase("PUT", "/rooms/living/window.json", "10");
             lastAutoWindowTime = now;
         }
      }
      
      lastTemp = t;
    }
  }

  if (now - lastReadTime > 1) {
    lastReadTime = now;
    
    static int pollingStep = 0;
    
    switch(pollingStep) {
      case 0: {
        String doorRaw = getFromFirebase("/smart_home/doors/main_door/position.json");
        String doorClean = cleanPayload(doorRaw);
        if (doorClean != "null" && doorClean != "") doorServo.write(doorClean.toInt());
        break;
      }
      case 1: {
        String winRaw = getFromFirebase("/rooms/living/window.json");
        String winClean = cleanPayload(winRaw);
        
        if (winClean != "null" && winClean != "") {
             int targetPos = 0;
             if (winClean == "true" || winClean == "True") targetPos = 60;
             else if (winClean == "false" || winClean == "False") targetPos = 10;
             else targetPos = winClean.toInt();
             
             if (targetPos < 5) targetPos = 5;
             if (targetPos > 175) targetPos = 175;
             
             windowServo.write(targetPos);
        }
        break;
      }
      case 2: {
        String ledLivRaw = getFromFirebase("/rooms/living/light.json");
        digitalWrite(PIN_LED_LIVING, parseBool(ledLivRaw) ? HIGH : LOW);
        break;
      }
      case 3: {
        String ledKitRaw = getFromFirebase("/rooms/kitchen/light.json");
        digitalWrite(PIN_LED_KITCHEN, parseBool(ledKitRaw) ? HIGH : LOW);
        break;
      }
      case 4: {
        String ledBedRaw = getFromFirebase("/rooms/bedroom/light.json");
        digitalWrite(PIN_LED_BEDROOM, parseBool(ledBedRaw) ? HIGH : LOW);
        break;
      }
    }
    
    pollingStep++;
    if (pollingStep > 4) pollingStep = 0;
  }
}

void checkButtonAndNotify() {
  unsigned long now = millis();
  if (now - lastNotificationTime < NOTIFICATION_COOLDOWN) {
    return;
  }
  
  delay(100); 
  
  String statusRaw = getFromFirebase("/smart_home/sensors/button/status.json");
  String status = cleanPayload(statusRaw);
  
  if (status == "known") {
      return;
  }
  
  String timestamp = String(millis());
  
  String notifJson = "{\"type\": \"unknown_button\", \"message\": \"Quelqu\'un a sonné !\", \"timestamp\": " + timestamp + ", \"read\": false}";
  
  String notifPath = "/smart_home/notifications/btn_" + timestamp + ".json";
  sendToFirebase("PUT", notifPath, notifJson);
  
  lastNotificationTime = now;
}

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
