import requests
import json
import time # ⭐ Fix: Added missing import
from firebase_admin import credentials, db, initialize_app

# ... (Configuration section remains the same)

# ================= MONITORING LOOP =================

START_TIME = time.time() * 1000 # Start time in ms

def monitor_notifications():
    """
    Monitor Firebase for new notifications and send FCM push
    """
    print("👀 Monitoring for notifications...")
    
    # 1. Listen to button notifications
    button_ref = db.reference('smart_home/notifications')
    
    def button_callback(event):
        try:
            # Check if event has data
            if not event.data:
                return

            # Handle initial data load (path is '/')
            if event.path == '/':
                if isinstance(event.data, dict):
                    # Check all existing children
                    for key, val in event.data.items():
                        process_button_data(val)
                return

            # Handle new child added (path is '/childKey')
            process_button_data(event.data)
            
        except Exception as e:
            print(f"Error processing button event: {e}")

    def process_button_data(data):
        if not isinstance(data, dict): return
        
        # Check type
        if data.get('type') != 'unknown_button': return
        
        # Check timestamp to ensure it's new (created after script start)
        ts = data.get('timestamp')
        if ts and int(ts) > START_TIME:
            print(f"\n🔔 New button notification detected! Time: {ts}")
            # Slightly update start time to avoid duplicates if rapid fire
            # But better to rely on idempotency. For now, simple check.
            send_button_alert()
        else:
            # Old notification
            pass

    # Using listen to child events is better for catching new additions
    button_ref.listen(button_callback)

    # 2. Listen to face alerts (legacy path)
    face_ref = db.reference('alerts')

    def face_callback(event):
        try:
             # Similar logic for faces
            if not event.data: return
            
            if event.path == '/':
                if isinstance(event.data, dict):
                    for key, val in event.data.items():
                        process_face_data(val)
                return

            process_face_data(event.data)
        except Exception as e:
            print(f"Error processing face event: {e}")

    def process_face_data(data):
        if not isinstance(data, dict): return
        
        ts = data.get('timestamp')
        if ts and int(ts) > START_TIME:
             print(f"\n🔔 New face notification detected! Time: {ts}")
             image_url = data.get('imageUrl')
             send_face_alert(image_url)

    face_ref.listen(face_callback)
    
    print("✅ Monitoring started for Buttons AND Faces (New events only). Press Ctrl+C to stop.")
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("\n👋 Stopping monitor...")

# ================= MAIN =================

if __name__ == '__main__':
    import time
    
    print("="*50)
    print("  FCM Push Notification Sender")
    print("="*50)
    
    # Test notification (optional)
    # send_fcm_notification("🎉 Test", "FCM is working!", "test")
    
    # Start monitoring
    monitor_notifications()
