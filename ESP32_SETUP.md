# ESP32 Setup Guide for Gas Detection and Door Control

## 📋 Required Hardware

### Gas Detection
- ESP32 Development Board
- MQ-2 or MQ-135 Gas Sensor
- Jumper wires
- Breadboard

### Door Control
- SG90 Servo Motor (small) OR MG996R (larger, needs external power)
- External 5-6V power supply (for MG996R)
- Jumper wires

## 🔌 Wiring Diagram

### Gas Sensor (MQ-2/MQ-135)
```
MQ Sensor    →    ESP32
---------          -----
VCC (5V)     →    5V
GND          →    GND
AO (Analog)  →    GPIO34 (ADC1_CH6)
```

### Servo Motor
```
Servo        →    ESP32 / Power
-----              --------------
Signal       →    GPIO18
VCC (Red)    →    5V (SG90) or External 5V (MG996R)
GND (Brown)  →    Common GND (connect ESP32 GND + External Power GND)
```

**⚠️ Important:** For larger servos like MG996R, use external power supply! The ESP32 cannot provide enough current.

## 📚 Required Arduino Libraries

Install these libraries via Arduino IDE Library Manager:

1. **FirebaseESP32** (by Mobizt)
   - Tools → Manage Libraries → Search "FirebaseESP32"
   
2. **ESP32Servo**
   - Tools → Manage Libraries → Search "ESP32Servo"

## ⚙️ Configuration Steps

### 1. Get Firebase Credentials

#### Firebase Project Setup:
1. Go to [Firebase Console](https://console.firebase.google.com/)
2. Select your project
3. Click **Realtime Database** from left menu
4. Copy your database URL (format: `project-id.firebaseio.com`)

#### Get Database Secret:
1. In Firebase Console, click gear icon ⚙️ → **Project Settings**
2. Go to **Service accounts** tab
3. Click **Database secrets**
4. Copy the secret key

#### Get User ID:
1. In Firebase Console, go to **Authentication**
2. Click on your user
3. Copy the **User UID**

### 2. Configure ESP32 Code

Open the `.ino` file and update these values:

```cpp
// WiFi credentials
#define WIFI_SSID "Your_WiFi_Name"
#define WIFI_PASSWORD "Your_WiFi_Password"

// Firebase credentials
#define FIREBASE_HOST "your-project-id.firebaseio.com"
#define FIREBASE_AUTH "your_database_secret_key_here"

// User ID
#define USER_ID "your_firebase_user_uid_here"
```

### 3. Firebase Realtime Database Rules

Set these rules in Firebase Console → Realtime Database → Rules:

```json
{
  "rules": {
    "users": {
      "$uid": {
        ".read": "$uid === auth.uid || auth.token.email_verified === true",
        ".write": "$uid === auth.uid || auth.token.email_verified === true"
      }
    }
  }
}
```

For testing only (less secure):
```json
{
  "rules": {
    ".read": true,
    ".write": true
  }
}
```

## 🚀 Upload Code to ESP32

### 1. Arduino IDE Setup

1. **Install ESP32 Board Support:**
   - File → Preferences
   - Additional Board Manager URLs: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
   - Tools → Board → Boards Manager → Search "ESP32" → Install

2. **Select Board:**
   - Tools → Board → ESP32 Arduino → Select your ESP32 model
   - Tools → Port → Select correct COM port

3. **Upload Settings:**
   - Upload Speed: 115200
   - Flash Frequency: 80MHz

### 2. Upload Process

1. Open the `.ino` file in Arduino IDE
2. Click **Verify** ✓ button to compile
3. Click **Upload** → button
4. Open **Serial Monitor** (Tools → Serial Monitor)
5. Set baud rate to **115200**

## 📊 Test Your Setup

### Gas Sensor Testing:
1. Upload code and open Serial Monitor
2. You should see readings every 2 seconds:
   ```
   --- Gas Sensor ---
   Level: 123 PPM | Status: safe
   ```
3. Bring a lighter near (don't light!) to see values change
4. Check Firebase Console to see real-time updates

### Door Control Testing:
1. Open your Flutter app
2. Go to Smart Access section
3. Use slider or buttons to control door
4. Servo should move smoothly
5. Check Serial Monitor for position updates:
   ```
   --- Door Control ---
   Moving to: 90°
   Status: partially_open
   ```

## 🔍 Troubleshooting

### WiFi Not Connecting:
- Check SSID and password spelling
- Ensure 2.4GHz WiFi (ESP32 doesn't support 5GHz)
- Move ESP32 closer to router

### Firebase Connection Failed:
- Verify database URL format (no `https://` or trailing `/`)
- Check database secret is correct
- Ensure Firebase Realtime Database is enabled

### Servo Not Moving:
- Check power supply (external power for MG996R)
- Verify GPIO18 connection
- Test servo with example sketch first

### Gas Sensor Always Shows 0:
- Check AO pin connection to GPIO34
- Sensor needs 24-48 hours warmup for accurate readings
- Try reading raw analog value first

## 📁 Project Files

Choose one of these based on your needs:

- **ESP32_Gas_Sensor.ino** - Gas sensor only
- **ESP32_Door_Control.ino** - Door control only
- **ESP32_Combined.ino** - Both features ✅ Recommended

## 🎯 Next Steps

1. ✅ Install required libraries
2. ✅ Wire up hardware
3. ✅ Configure credentials in code
4. ✅ Upload to ESP32
5. ✅ Test with Flutter app
6. 🎉 Enjoy your smart home!

## 📞 Need Help?

Check Serial Monitor output for detailed error messages and debugging information.
