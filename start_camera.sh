#!/bin/bash
# Camera Server Startup Script with Port Cleanup
# This script ensures port 8082 is free before starting the server

echo "🔧 Camera Server Startup Script"
echo "================================"

# Kill any process using port 8082
echo "Checking port 8082..."
sudo fuser -k 8082/tcp 2>/dev/null

# Wait for port to be released
sleep 1

# Fix camera permissions
echo "Setting camera permissions..."
sudo chmod 666 /dev/video* 2>/dev/null

# Start the server
echo "Starting camera server..."
python3 camera_server_v2.py
