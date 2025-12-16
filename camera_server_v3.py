#!/usr/bin/env python3
import cv2
from flask import Flask, Response
import time
import threading

app = Flask(__name__)

# Global variables
camera = None
camera_lock = threading.Lock()
last_frame = None
frame_lock = threading.Lock()
camera_thread = None
camera_running = False

def camera_capture_thread():
    """Background thread that continuously captures frames"""
    global camera, last_frame, camera_running
    
    print("📹 Starting camera capture thread...")
    
    while camera_running:
        with camera_lock:
            if camera is None:
                camera = init_camera()
                if camera is None:
                    time.sleep(1)
                    continue
            
            try:
                success, frame = camera.read()
                if not success:
                    print("⚠ Failed to read frame, reinitializing camera...")
                    camera.release()
                    camera = None
                    time.sleep(0.5)
                    continue
                
                # Encode frame to JPEG
                ret, buffer = cv2.imencode('.jpg', frame, [cv2.IMWRITE_JPEG_QUALITY, 80])
                if ret:
                    with frame_lock:
                        last_frame = buffer.tobytes()
                        
            except Exception as e:
                print(f"⚠ Error in camera capture: {e}")
                if camera is not None:
                    camera.release()
                    camera = None
                time.sleep(0.5)
        
        time.sleep(0.033)  # ~30 FPS
    
    # Cleanup
    with camera_lock:
        if camera is not None:
            camera.release()
            camera = None
    print("✓ Camera capture thread stopped")

def init_camera():
    """Initialize camera with retry logic"""
    for i in range(3):
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
    
    print("✗ Failed to initialize camera after 3 attempts")
    return None

def generate_frames():
    """Generate video frames for each client"""
    print("👤 New client connected")
    
    while True:
        with frame_lock:
            if last_frame is None:
                # Send a small placeholder if no frame available
                time.sleep(0.1)
                continue
            
            frame_bytes = last_frame
        
        yield (b'--frame\r\n'
               b'Content-Type: image/jpeg\r\n\r\n' + frame_bytes + b'\r\n')
        
        time.sleep(0.033)  # ~30 FPS

@app.route('/')
def video_feed():
    """Video streaming route"""
    return Response(generate_frames(),
                    mimetype='multipart/x-mixed-replace; boundary=frame')

@app.route('/status')
def status():
    """Status endpoint"""
    with camera_lock:
        camera_active = camera is not None
    
    with frame_lock:
        has_frames = last_frame is not None
    
    return {
        "camera_active": camera_active,
        "has_frames": has_frames,
        "status": "running"
    }

@app.route('/health')
def health():
    """Health check endpoint"""
    return {"status": "ok"}

def start_camera_thread():
    """Start the background camera capture thread"""
    global camera_thread, camera_running
    
    camera_running = True
    camera_thread = threading.Thread(target=camera_capture_thread, daemon=True)
    camera_thread.start()

if __name__ == '__main__':
    print("=" * 60)
    print("📹 Multi-Client Camera Server v3")
    print("=" * 60)
    print("Starting on http://0.0.0.0:8082")
    print("📱 External URL: http://192.168.100.152:8082")
    print("Supports multiple simultaneous connections!")
    print("=" * 60)
    
    # Start camera capture thread
    start_camera_thread()
    
    try:
        app.run(host='0.0.0.0', port=8082, debug=False, threaded=True)
    finally:
        camera_running = False
        if camera_thread is not None:
            camera_thread.join(timeout=2)
        print("\n✓ Server stopped")
