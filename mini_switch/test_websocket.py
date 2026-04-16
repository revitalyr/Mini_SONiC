#!/usr/bin/env python3
"""
Test WebSocket Client
"""

import asyncio
import websockets
import json

async def test_websocket():
    try:
        print("Attempting to connect to WebSocket...")
        uri = "ws://localhost:8765"
        async with websockets.connect(uri) as websocket:
            print(f"Connected to {uri}")
            
            # Send test message
            test_message = {
                "type": "test",
                "message": "Hello from test client"
            }
            await websocket.send(json.dumps(test_message))
            print("Test message sent")
            
            # Wait for responses
            try:
                async for message in websocket:
                    data = json.loads(message)
                    print(f"Received: {data}")
                    if data.get("type") == "echo":
                        print("Echo response received - WebSocket working!")
                        break
            except websockets.exceptions.ConnectionClosed:
                print("Connection closed")
                break
                
    except Exception as e:
        print(f"WebSocket connection failed: {e}")

if __name__ == "__main__":
    asyncio.run(test_websocket())
