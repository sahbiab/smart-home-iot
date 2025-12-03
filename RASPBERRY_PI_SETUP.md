# Raspberry Pi Setup Guide

## 📋 Prerequisites

Install required packages on your Raspberry Pi:

```bash
sudo apt update
sudo apt install python3-pip -y
pip3 install flask flask-cors
```

## 🚀 Quick Start

### 1. Transfer the Server File

Copy `raspberry_pi_server.py` to your Raspberry Pi:

```bash
# From your PC, use SCP (replace 'pi' with your username)
scp raspberry_pi_server.py pi@192.168.137.160:/home/pi/

# Or use a USB drive, or copy-paste the content
```

### 2. Run the Server

On your Raspberry Pi:

```bash
cd /home/pi
python3 raspberry_pi_server.py
```

You should see:
```
============================================================
🍓 Raspberry Pi Face Recognition Server
============================================================
📁 Data directory: /home/pi/face_data/faces
🌐 Server starting on http://0.0.0.0:5000
📱 Mobile app should connect to: http://192.168.137.160:5000
============================================================

Waiting for image uploads...
```

### 3. Test from Your Phone

1. Open the app on your phone
2. Tap "Add Person"
3. Enter a name (e.g., "John")
4. Capture the 5 images
5. Watch the Raspberry Pi terminal - you should see:
   ```
   ✓ Saved: /home/pi/face_data/faces/John/center.jpg
   ✓ Saved: /home/pi/face_data/faces/John/up.jpg
   ✓ Saved: /home/pi/face_data/faces/John/down.jpg
   ✓ Saved: /home/pi/face_data/faces/John/left.jpg
   ✓ Saved: /home/pi/face_data/faces/John/right.jpg
   
   [2025-12-03 00:27:00] Received 5 images for 'John'
   Saved to: /home/pi/face_data/faces/John
   ```

### 4. Check Saved Images

```bash
ls -la /home/pi/face_data/faces/
```

You should see folders for each person with their images inside!

## 🔧 Troubleshooting

### Server won't start?
```bash
# Check if port 5000 is already in use
sudo lsof -i :5000

# Kill any process using port 5000
sudo kill -9 <PID>
```

### Can't connect from phone?
```bash
# Check firewall (if enabled)
sudo ufw allow 5000

# Verify IP address
hostname -I
```

### Check server is running
From your PC or phone browser, visit:
```
http://192.168.137.160:5000
```

You should see:
```json
{
  "service": "Raspberry Pi Face Recognition Server",
  "status": "running",
  "endpoints": {...}
}
```

## 🔄 Auto-Start on Boot (Optional)

Create a systemd service:

```bash
sudo nano /etc/systemd/system/face-recognition.service
```

Add:
```ini
[Unit]
Description=Face Recognition Server
After=network.target

[Service]
Type=simple
User=pi
WorkingDirectory=/home/pi
ExecStart=/usr/bin/python3 /home/pi/raspberry_pi_server.py
Restart=always

[Install]
WantedBy=multi-user.target
```

Enable and start:
```bash
sudo systemctl enable face-recognition.service
sudo systemctl start face-recognition.service
sudo systemctl status face-recognition.service
```

## 📁 Data Structure

Images are saved as:
```
/home/pi/face_data/faces/
├── John/
│   ├── center.jpg
│   ├── up.jpg
│   ├── down.jpg
│   ├── left.jpg
│   └── right.jpg
├── Sarah/
│   ├── center.jpg
│   ├── up.jpg
│   └── ...
└── ...
```

Ready to use for training your face recognition model!
