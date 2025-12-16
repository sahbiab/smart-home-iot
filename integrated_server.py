#!/usr/bin/env python3
import cv2
import face_recognition
import requests
import os
import numpy as np
import time
import threading
from flask import Flask, Response
import signal
import sys
import socket
from werkzeug.serving import make_server

# --- CONFIGURATION (Firebase) ---
FIREBASE_URL = "https://smarthome-dbadb-default-rtdb.firebaseio.com"
FIREBASE_SECRET = "16VEgR6yMDETwAfo4SXUkGvVg06vL9CnFmKX3O4V"
KNOWN_FACES_DIR = os.path.join(os.path.expanduser("~"), "face_data", "faces")
DOOR_OPEN_POS = 180
DOOR_CLOSE_POS = 0

# --- GLOBAL STATE ---
camera = None
camera_lock = threading.Lock()
last_frame_jpeg = None  # For streaming (MJPEG)
current_frame_rgb = None # For recognition
frame_lock = threading.Lock()
running = True

# Initialize Flask
app = Flask(__name__)

# --- FACE RECOGNITION LOGIC ---
known_face_encodings = []
known_face_names = []

def load_known_faces():
    global known_face_encodings, known_face_names
    print(f"📂 Loading faces from: {KNOWN_FACES_DIR}")
    if not os.path.exists(KNOWN_FACES_DIR):
        os.makedirs(KNOWN_FACES_DIR, exist_ok=True)
        return

    for person_name in os.listdir(KNOWN_FACES_DIR):
        person_dir = os.path.join(KNOWN_FACES_DIR, person_name)
        if not os.path.isdir(person_dir): continue

        for filename in os.listdir(person_dir):
            if filename.lower().endswith(('.png', '.jpg', '.jpeg')):
                try:
                    image = face_recognition.load_image_file(os.path.join(person_dir, filename))
                    encodings = face_recognition.face_encodings(image)
                    if len(encodings) > 0:
                        known_face_encodings.append(encodings[0])
                        known_face_names.append(person_name)
                        print(f"  ✓ Loaded: {person_name}")
                except Exception as e:
                    print(f"  ✗ Error loading {filename}: {e}")
    print(f"✅ Total faces: {len(known_face_names)}")

def open_door_task():
    """Background task to open door"""
    try:
        url = f"{FIREBASE_URL}/smart_home/doors/main_door/position.json?auth={FIREBASE_SECRET}"
        print("🔓 [DOOR] Opening...")
        requests.put(url, json=DOOR_OPEN_POS)
        time.sleep(5)
        print("🔒 [DOOR] Closing...")
        requests.put(url, json=DOOR_CLOSE_POS)
    except Exception as e:
        print(f"⚠ [DOOR] Error: {e}")

# --- CAMERA THREAD ---
def camera_loop():
    global camera, last_frame_jpeg, current_frame_rgb, running
    
    # 1. Load Data
    load_known_faces()
    
    # 2. Init Camera
    print("📷 Starting Camera...")
    camera = cv2.VideoCapture(0)
    camera.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
    camera.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
    
    process_this_frame = True
    
    while running:
        success, frame = camera.read()
        if not success:
            print("❌ Camera read failed")
            time.sleep(1)
            camera.release()
            camera = cv2.VideoCapture(0) # Retry
            continue

        # A. Face Recognition (Every other frame)
        if process_this_frame:
            small_frame = cv2.resize(frame, (0, 0), fx=0.25, fy=0.25)
            # OpenCV uses BGR, face_recognition uses RGB
            rgb_small_frame = small_frame[:, :, ::-1] # BGR to RGB
            
            # Find faces
            face_locations = face_recognition.face_locations(rgb_small_frame)
            face_encodings = face_recognition.face_encodings(rgb_small_frame, face_locations)

            face_names_found = []
            for face_encoding in face_encodings:
                matches = face_recognition.compare_faces(known_face_encodings, face_encoding, tolerance=0.6)
                name = "Unknown"
                if True in matches:
                    first_match_index = matches.index(True)
                    name = known_face_names[first_match_index]
                    
                    # Trigger Door (if not recently triggered)
                    # For simplicity, we just fire it in a thread
                    threading.Thread(target=open_door_task, daemon=True).start()
                
                face_names_found.append(name)
        
        process_this_frame = not process_this_frame

        # B. Draw Rectangles (Optional, mostly for debug/monitor)
        # Note: We rely on `face_locations` from previous frame if skipped
        # But here we just won't draw if we skipped processing to keep it simple or use last known locations
        # For simplicity in this merged script, we will skip drawing on the stream to save CPU
        # or we just draw if we have locations.
        
        # C. Encode for Streaming
        ret, buffer = cv2.imencode('.jpg', frame)
        if ret:
            with frame_lock:
                last_frame_jpeg = buffer.tobytes()
        
        time.sleep(0.01)

    camera.release()

# --- FLASK ROUTES ---
@app.route('/')
def video_feed():
    def generate():
        while running:
            with frame_lock:
                if last_frame_jpeg is None:
                    time.sleep(0.1)
                    continue
                frame_data = last_frame_jpeg
            
            yield (b'--frame\r\n'
                   b'Content-Type: image/jpeg\r\n\r\n' + frame_data + b'\r\n')
            time.sleep(0.04) # Limit stream FPS
            
    return Response(generate(), mimetype='multipart/x-mixed-replace; boundary=frame')

# --- MAIN ---
if __name__ == '__main__':
    # Start Camera Thread
    t = threading.Thread(target=camera_loop, daemon=True)
    t.start()
    
    print("="*40)
    print("🚀 INTEGRATED SERVER STARTED")
    print("   - Face Recognition: ACTIVE")
    print("   - Camera Stream: http://0.0.0.0:8082")
    print("="*40)
    
    try:
        # Run Flask on port 8082 (Stream Port)
        app.run(host='0.0.0.0', port=8082, debug=False, threaded=True)
    except KeyboardInterrupt:
        pass
    finally:
        running = False
        print("Stopped.")
