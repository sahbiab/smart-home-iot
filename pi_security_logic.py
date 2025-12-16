import firebase_admin
from firebase_admin import credentials, db, storage
import time
import datetime
# import RPi.GPIO as GPIO # Uncomment on Pi
# import cv2 # Open CV for camera
# import face_recognition # Face Recognition library

# ================= CONFIGURATION =================
# Path to your service account key file (User must generate this)
CRED_PATH = 'serviceAccountKey.json'
DATABASE_URL = 'https://YOUR_PROJECT_ID-default-rtdb.firebaseio.com/'
BUCKET_NAME = 'YOUR_PROJECT_ID.appspot.com'

# Hardware Pins (Example)
TRIGGER_PIN = 23
ECHO_PIN = 24

# Thresholds
DIST_THRESHOLD = 80 # cm

# ================= FIREBASE SETUP =================
try:
    cred = credentials.Certificate(CRED_PATH)
    firebase_admin.initialize_app(cred, {
        'databaseURL': DATABASE_URL,
        'storageBucket': BUCKET_NAME
    })
    print("Firebase Initialized")
except Exception as e:
    print(f"Error initializing Firebase: {e}")
    print("Ensure 'serviceAccountKey.json' is in the same folder.")

# ================= HELPER FUNCTIONS =================

def upload_image(image_path):
    """Uploads image to Firebase Storage and returns the public URL."""
    try:
        bucket = storage.bucket()
        blob = bucket.blob(f"alerts/{int(time.time())}.jpg")
        blob.upload_from_filename(image_path)
        blob.make_public()
        print(f"Image uploaded: {blob.public_url}")
        return blob.public_url
    except Exception as e:
        print(f"Upload failed: {e}")
        return None

def send_alert(image_url):
    """Writes alert data to Realtime Database."""
    try:
        ref = db.reference('alerts')
        ref.push({
            'timestamp': int(time.time() * 1000), # Milliseconds for Flutter
            'imageUrl': image_url,
            'message': 'Unknown Person Detected via Ultrasonic Trigger'
        })
        print("Alert sent to Database")
    except Exception as e:
        print(f"Database write failed: {e}")

def get_distance():
    """Reads distance from Ultrasonic Sensor."""
    # Placeholder Logic
    # GPIO.output(TRIGGER_PIN, True)
    # time.sleep(0.00001)
    # GPIO.output(TRIGGER_PIN, False)
    # ... calculation ...
    return 100 # Mock distance

def capture_and_recognize():
    """Captures image and checks for unknown face."""
    # Placeholder Logic
    print("Capturing image...")
    # cv2.imwrite("capture.jpg", frame)
    
    # Face Recognition Logic
    # ...
    is_unknown = True # Mock result
    
    if is_unknown:
        print("Unknown face detected!")
        # Upload and Alert
        # url = upload_image("capture.jpg")
        # if url:
        #     send_alert(url)
    return

# ================= MAIN LOOP =================

def main():
    print("Starting Security System...")
    try:
        while True:
            dist = get_distance()
            if dist < DIST_THRESHOLD:
                print(f"Object detected at {dist}cm")
                capture_and_recognize()
                time.sleep(5) # Debounce
            
            time.sleep(0.1)
            
    except KeyboardInterrupt:
        print("Stopping...")
        # GPIO.cleanup()

if __name__ == '__main__':
    main()
