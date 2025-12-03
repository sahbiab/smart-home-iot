#!/usr/bin/env python3
"""
Raspberry Pi Server for Face Recognition
Receives images from Flutter app and saves them for processing
"""

from flask import Flask, request, jsonify
from flask_cors import CORS
import base64
import os
from datetime import datetime

app = Flask(__name__)
CORS(app)  # Enable CORS for mobile app

# Directory to save received images
FACE_DATA_DIR = "/home/pi/face_data/faces"

# Create base directory if it doesn't exist
os.makedirs(FACE_DATA_DIR, exist_ok=True)

@app.route('/api/upload_person', methods=['POST'])
def upload_person():
    """
    Receive person's face images from mobile app
    Expected JSON format:
    {
        "name": "person_name",
        "images": {
            "center": "base64_encoded_image",
            "up": "base64_encoded_image",
            "down": "base64_encoded_image",
            "left": "base64_encoded_image",
            "right": "base64_encoded_image"
        }
    }
    """
    try:
        data = request.get_json()
        
        if not data:
            return jsonify({"error": "No data received"}), 400
        
        name = data.get('name')
        images = data.get('images')
        
        if not name or not images:
            return jsonify({"error": "Missing name or images"}), 400
        
        # Create person directory
        person_dir = os.path.join(FACE_DATA_DIR, name)
        os.makedirs(person_dir, exist_ok=True)
        
        # Save each image
        saved_count = 0
        for image_name, base64_data in images.items():
            try:
                # Decode base64 image
                image_bytes = base64.b64decode(base64_data)
                
                # Save image
                image_path = os.path.join(person_dir, f"{image_name}.jpg")
                with open(image_path, 'wb') as f:
                    f.write(image_bytes)
                
                saved_count += 1
                print(f"✓ Saved: {image_path}")
                
            except Exception as e:
                print(f"✗ Error saving {image_name}: {e}")
        
        # Log the upload
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        print(f"\n[{timestamp}] Received {saved_count} images for '{name}'")
        print(f"Saved to: {person_dir}\n")
        
        return jsonify({
            "success": True,
            "message": f"Successfully saved {saved_count} images for {name}",
            "person": name,
            "images_saved": saved_count,
            "directory": person_dir
        }), 200
        
    except Exception as e:
        print(f"ERROR: {e}")
        return jsonify({"error": str(e)}), 500


@app.route('/api/status', methods=['GET'])
def get_status():
    """Get server status and list of registered persons"""
    try:
        # Get list of persons (directories in face_data)
        persons = []
        if os.path.exists(FACE_DATA_DIR):
            persons = [d for d in os.listdir(FACE_DATA_DIR) 
                      if os.path.isdir(os.path.join(FACE_DATA_DIR, d))]
        
        return jsonify({
            "status": "online",
            "persons_count": len(persons),
            "persons": persons,
            "data_directory": FACE_DATA_DIR
        }), 200
        
    except Exception as e:
        return jsonify({"error": str(e)}), 500


@app.route('/api/train', methods=['POST'])
def train_model():
    """Trigger model training (placeholder for future implementation)"""
    return jsonify({
        "message": "Training endpoint - to be implemented",
        "status": "not_implemented"
    }), 501


@app.route('/', methods=['GET'])
def home():
    """Root endpoint"""
    return jsonify({
        "service": "Raspberry Pi Face Recognition Server",
        "status": "running",
        "endpoints": {
            "upload": "/api/upload_person (POST)",
            "status": "/api/status (GET)",
            "train": "/api/train (POST)"
        }
    }), 200


if __name__ == '__main__':
    print("=" * 60)
    print("🍓 Raspberry Pi Face Recognition Server")
    print("=" * 60)
    print(f"📁 Data directory: {FACE_DATA_DIR}")
    print(f"🌐 Server starting on http://0.0.0.0:5000")
    print(f"📱 Mobile app should connect to: http://192.168.137.160:5000")
    print("=" * 60)
    print("\nWaiting for image uploads...\n")
    
    # Run server on all interfaces (0.0.0.0) so it's accessible from network
    app.run(host='0.0.0.0', port=5000, debug=True)
