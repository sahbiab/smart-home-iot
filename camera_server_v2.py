#!/usr/bin/env python3
import cv2
from flask import Flask, Response
import time

app = Flask(__name__)

# Global camera object
camera = None

def init_camera():
    """Initialize camera with retry logic"""
    global camera
    if camera is not None:
        camera.release()
    
    for i in range(5):  # Try 5 times
        try:
            cam = cv2.VideoCapture(0)
            cam.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
            cam.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
            cam.set(cv2.CAP_PROP_FPS, 15)
            cam.set(cv2.CAP_PROP_BUFFERSIZE, 1)
            
            # Test if camera works
            ret, frame = cam.read()
            if ret:
                print(f"✓ Camera initialized successfully on attempt {i+1}")
                return cam
            else:
                cam.release()
        except Exception as e:
            print(f"✗ Camera init attempt {i+1} failed: {e}")
        
        time.sleep(0.5)
    
    print("✗ Failed to initialize camera after 5 attempts")
    return None

def generate_frames():
    """Generate video frames with error recovery"""
    global camera
    
    while True:
        if camera is None:
            camera = init_camera()
            if camera is None:
                # Send a black frame with error message
                yield (b'--frame\r\n'
                       b'Content-Type: image/jpeg\r\n\r\n' + 
                       b'\xff\xd8\xff\xe0\x00\x10JFIF' + b'\x00' * 100 + 
                       b'\r\n')
                time.sleep(1)
                continue
        
        try:
            success, frame = camera.read()
            if not success:
                print("⚠ Failed to read frame, reinitializing camera...")
                camera.release()
                camera = None
                time.sleep(0.1)
                continue
            
            # Encode frame
            ret, buffer = cv2.imencode('.jpg', frame, [cv2.IMWRITE_JPEG_QUALITY, 80])
            if not ret:
                continue
                
            frame_bytes = buffer.tobytes()
            yield (b'--frame\r\n'
                   b'Content-Type: image/jpeg\r\n\r\n' + frame_bytes + b'\r\n')
            
            time.sleep(0.033)  # ~30 FPS
            
        except Exception as e:
            print(f"⚠ Error in frame generation: {e}")
            if camera is not None:
                camera.release()
                camera = None
            time.sleep(0.5)

@app.route('/')
def video_feed():
    return Response(generate_frames(),
                    mimetype='multipart/x-mixed-replace; boundary=frame')

@app.route('/status')
def status():
    global camera
    return {
        "camera_active": camera is not None,
        "status": "running"
    }

if __name__ == '__main__':
    print("=" * 60)
    print("📹 Improved Camera Server v2")
    print("=" * 60)
    print("Starting on http://0.0.0.0:8081")
    print("=" * 60)
    
    # Initialize camera on startup
    camera = init_camera()
    
    try:
        app.run(host='0.0.0.0', port=8081, debug=False, threaded=True)
    finally:
        if camera is not None:
            camera.release()
            print("\n✓ Camera released")
