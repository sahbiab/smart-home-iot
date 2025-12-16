import cv2
import face_recognition
import requests
import os
import numpy as np
import time

# --- CONFIGURATION ---
# This must match your Arduino Configuration
FIREBASE_URL = "https://smarthome-dbadb-default-rtdb.firebaseio.com"
FIREBASE_SECRET = "16VEgR6yMDETwAfo4SXUkGvVg06vL9CnFmKX3O4V"

# Path to the folder containing known faces (from the Upload App)
# Using dynamic path to avoid permission errors
KNOWN_FACES_DIR = os.path.join(os.path.expanduser("~"), "face_data", "faces")

# Configuration for door control
DOOR_OPEN_POS = 180
DOOR_CLOSE_POS = 0
OPEN_DURATION = 20  # Seconds

def load_known_faces():
    """
    Load all valid images from the KNOWN_FACES_DIR.
    Returns:
        known_encodings (list): List of face encodings
        known_names (list): List of names corresponding to encodings
    """
    known_encodings = []
    known_names = []

    print(f"📂 Loading faces from: {KNOWN_FACES_DIR}")
    
    if not os.path.exists(KNOWN_FACES_DIR):
        print(f"⚠ Warning: Directory {KNOWN_FACES_DIR} does not exist.")
        os.makedirs(KNOWN_FACES_DIR, exist_ok=True)
        return [], []

    # Iterate through each person's folder
    for person_name in os.listdir(KNOWN_FACES_DIR):
        person_dir = os.path.join(KNOWN_FACES_DIR, person_name)
        
        if not os.path.isdir(person_dir):
            continue

        # Look for images in the person's folder
        for filename in os.listdir(person_dir):
            if filename.lower().endswith(('.png', '.jpg', '.jpeg')):
                image_path = os.path.join(person_dir, filename)
                try:
                    # Load image
                    image = face_recognition.load_image_file(image_path)
                    # Encode face
                    encodings = face_recognition.face_encodings(image)
                    
                    if len(encodings) > 0:
                        known_encodings.append(encodings[0])
                        known_names.append(person_name)
                        print(f"  ✓ Loaded: {person_name} ({filename})")
                    else:
                        print(f"  ⚠ No face found in {filename}")
                except Exception as e:
                    print(f"  ✗ Error loading {filename}: {e}")

    print(f"✅ Total known faces loaded: {len(known_names)}")
    return known_encodings, known_names

def open_door():
    """Send command to Firebase to open the door"""
    print("🔓 AUTHENTICATED! Opening Door...")
    
    url = f"{FIREBASE_URL}/smart_home/doors/main_door/position.json?auth={FIREBASE_SECRET}"
    
    try:
        # 1. Open Door
        response = requests.put(url, json=DOOR_OPEN_POS)
        if response.status_code == 200:
            print(f"  -> Command Sent: OPEN ({DOOR_OPEN_POS})")
        else:
            print(f"  -> Error sending command: {response.text}")

        # 2. Wait
        time.sleep(OPEN_DURATION)

        # 3. Close Door
        print("🔒 Timeout. Closing Door...")
        requests.put(url, json=DOOR_CLOSE_POS)
        print(f"  -> Command Sent: CLOSED ({DOOR_CLOSE_POS})")
        
    except Exception as e:
        print(f"  ✗ Connection Error: {e}")

def main():
    # 1. Load Data
    known_face_encodings, known_face_names = load_known_faces()
    
    if not known_face_encodings:
        print("❌ No known faces found. Please add people using the App first.")
        # We continue anyway to show the camera stream, but recognition won't work
    
    # 2. Initialize Camera
    print("📷 Starting Camera...")
    video_capture = cv2.VideoCapture(0)
    
    # Optimization variables
    process_this_frame = True
    
    while True:
        # Grab a single frame
        ret, frame = video_capture.read()
        if not ret:
            print("❌ Failed to grab frame")
            break

        # Resize for speed (1/4 size)
        small_frame = cv2.resize(frame, (0, 0), fx=0.25, fy=0.25)
        # Convert BGR (OpenCV) to RGB (face_recognition)
        rgb_small_frame = small_frame[:, :, ::-1]

        # Only process every other frame (optimization)
        if process_this_frame:
            # Find all faces in current frame
            face_locations = face_recognition.face_locations(rgb_small_frame)
            face_encodings = face_recognition.face_encodings(rgb_small_frame, face_locations)

            face_names = []
            for face_encoding in face_encodings:
                # See if the face is a match for the known face(s)
                matches = face_recognition.compare_faces(known_face_encodings, face_encoding, tolerance=0.6)
                name = "Unknown"

                # Use the known face with the smallest distance to the new face
                face_distances = face_recognition.face_distance(known_face_encodings, face_encoding)
                if len(face_distances) > 0:
                    best_match_index = np.argmin(face_distances)
                    if matches[best_match_index]:
                        name = known_face_names[best_match_index]
                        
                        # --- ACTION TRIGGER ---
                        # If a known person is found, Open the Door
                        # To prevent spamming, you might want to add a cooldown here
                        open_door()
                        # Simple "Debounce" sleep to wait for door cycle to finish
                        # The loop will pause here for 5 seconds inside open_door()
                        
                face_names.append(name)

        process_this_frame = not process_this_frame

        # Display Logic (Optional - draws boxes on screen if monitor attached)
        for (top, right, bottom, left), name in zip(face_locations, face_names):
            # Scale back up x4
            top *= 4
            right *= 4
            bottom *= 4
            left *= 4

            # Draw box
            cv2.rectangle(frame, (left, top), (right, bottom), (0, 255, 0), 2)
            # Draw Label
            cv2.rectangle(frame, (left, bottom - 35), (right, bottom), (0, 255, 0), cv2.FILLED)
            font = cv2.FONT_HERSHEY_DUPLEX
            cv2.putText(frame, name, (left + 6, bottom - 6), font, 1.0, (255, 255, 255), 1)

        # Show Result
        cv2.imshow('Smart Home Face ID', frame)

        # Hit 'q' to quit
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

    video_capture.release()
    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()
