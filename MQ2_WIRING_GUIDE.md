# 🔌 MQ-2 Gas Sensor Wiring Guide for ESP32

## 📸 MQ-2 Sensor Overview

The MQ-2 gas sensor typically has **4 pins**:

```
┌─────────────────┐
│                 │
│     MQ-2        │
│   Gas Sensor    │
│                 │
└─┬─┬─┬─┬─────────┘
  │ │ │ │
  │ │ │ └─ AO (Analog Output)
  │ │ └─── DO (Digital Output) - Optional, not used
  │ └───── GND
  └─────── VCC
```

## ✅ Wiring Connections

### MQ-2 → ESP32

| MQ-2 Pin | ESP32 Pin | Description |
|----------|-----------|-------------|
| **VCC** | **5V** or **VIN** | Power supply (needs 5V) |
| **GND** | **GND** | Ground |
| **AO** | **GPIO34** | Analog output signal |
| **DO** | Not connected | Digital output (not needed) |

### 🔴 Important Notes:
- ⚠️ **MQ-2 needs 5V** to work properly (not 3.3V)
- ✅ **GPIO34 is safe** - it's on ADC1 and can read 0-3.3V analog signals
- 🔥 The sensor will heat up - this is normal!

## 📐 Detailed Wiring Diagram

```
ESP32                          MQ-2 Gas Sensor
┌────────────┐                 ┌──────────────┐
│            │                 │              │
│        5V  ├─────────────────┤ VCC          │
│            │     (Red wire)  │              │
│            │                 │              │
│       GND  ├─────────────────┤ GND          │
│            │   (Black wire)  │              │
│            │                 │              │
│    GPIO34  ├─────────────────┤ AO           │
│   (ADC1_6) │  (Yellow wire)  │ (Analog Out) │
│            │                 │              │
│            │              ╳──┤ DO           │
│            │           (Not  │ (Not used)   │
└────────────┘          needed)└──────────────┘
```

## 🎨 Wire Color Recommendations

| Connection | Suggested Color |
|------------|----------------|
| VCC (5V) | 🔴 Red |
| GND | ⚫ Black |
| AO (Analog) | 🟡 Yellow or Green |

## 🔧 Step-by-Step Connection

### Step 1: Identify MQ-2 Pins
Look at the back of your MQ-2 sensor module. The pins are usually labeled:
```
VCC  GND  DO  AO
```

### Step 2: Connect Power (VCC)
Connect **VCC** on MQ-2 to **5V** (or **VIN**) pin on ESP32
- Use a **red wire**
- ⚠️ Must be 5V, not 3.3V!

### Step 3: Connect Ground (GND)
Connect **GND** on MQ-2 to **GND** pin on ESP32
- Use a **black wire**

### Step 4: Connect Analog Output (AO)
Connect **AO** on MQ-2 to **GPIO34** on ESP32
- Use a **yellow wire**
- GPIO34 is safe for analog reading (0-3.3V)

### Step 5: Leave DO Disconnected
- The **DO** (Digital Output) pin is not needed for this project
- Leave it unconnected

## 🖼️ Physical Connection Example

```
Looking at ESP32 from top:
                    
     ╔════════════════════╗
     ║  ESP32 Dev Board   ║
     ║                    ║
 5V  ║ ●──────────────────║──● VCC (MQ-2)
     ║                    ║
GND  ║ ●──────────────────║──● GND (MQ-2)
     ║                    ║
GPIO ║                    ║
34   ║ ●──────────────────║──● AO (MQ-2)
     ║                    ║
     ║                    ║
     ╚════════════════════╝
```

## ⚡ Power Supply Considerations

### Option 1: USB Power (Recommended for testing)
- Connect ESP32 via USB
- MQ-2 draws ~150mA, USB can provide 500mA
- ✅ Usually sufficient

### Option 2: External Power Supply
If you're also powering a servo motor:
```
External 5V Supply (2A)
         │
         ├─────→ ESP32 VIN
         ├─────→ MQ-2 VCC
         ├─────→ Servo VCC
         │
        GND ────→ Common Ground (ESP32 + MQ-2 + Servo)
```

## 🔥 Sensor Warm-Up Period

⚠️ **Important:** The MQ-2 sensor needs to warm up before giving accurate readings!

- **First use:** 24-48 hours preheating recommended
- **Regular use:** 2-5 minutes warm-up each time
- The sensor will be **hot** - this is normal!

## 🧪 Testing the Connection

### 1. Upload Test Code
In Arduino IDE, upload this simple test:

```cpp
void setup() {
  Serial.begin(115200);
  pinMode(34, INPUT);
}

void loop() {
  int sensorValue = analogRead(34);
  Serial.print("MQ-2 Value: ");
  Serial.println(sensorValue);
  delay(1000);
}
```

### 2. Open Serial Monitor
- Set baud rate to **115200**
- You should see values like: `MQ-2 Value: 500` (varies based on gas concentration)

### 3. Test Gas Detection
- Spray some perfume, alcohol, or lighter gas near the sensor
- The value should **increase** significantly
- Normal air: ~200-500
- Gas detected: 1000-4000

## ✅ Verification Checklist

- [ ] VCC connected to 5V (not 3.3V)
- [ ] GND connected to ESP32 ground
- [ ] AO connected to GPIO34
- [ ] DO pin left unconnected
- [ ] Sensor is heating up (warm to touch)
- [ ] Serial monitor shows changing values
- [ ] Values increase when gas is present

## 🛠️ Troubleshooting

### Problem: Sensor not heating up
- ✅ Check VCC is connected to 5V (not 3.3V)
- ✅ Check GND connection

### Problem: Always reading 0 or 4095
- ✅ Check analog pin is GPIO34
- ✅ Sensor may need warm-up time
- ✅ Check AO wire connection

### Problem: Values don't change with gas
- ✅ Wait for sensor to warm up (5 minutes)
- ✅ Sensor may need 24-48h initial heating
- ✅ Ensure gas is near the sensor mesh

## 📊 Expected Values

```
Clean Air:       200 - 600
Light Smoke:     600 - 1500
Heavy Smoke:     1500 - 3000
LPG/Gas Leak:    2000 - 4000
Maximum:         4095
```

---

## 🎯 Your Code Configuration

In your **ESP32_Smart_Home_Complete.ino**, the sensor is already configured:

```cpp
#define GAS_SENSOR_PIN 34  // ← This is correct!
```

The code reads it with:
```cpp
int sensorValue = analogRead(GAS_SENSOR_PIN);
```

You're all set! Just wire it up as shown above! 🚀
