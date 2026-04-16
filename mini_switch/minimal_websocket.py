#!/usr/bin/env python3
"""
Minimal WebSocket Server
"""

import asyncio
import websockets
import json

async def handler(websocket, path):
    print(f"Client connected from {path}")
    websocket_clients.add(websocket)
    
    try:
        async for message in websocket:
            data = json.loads(message)
            print(f"Received: {data}")
            
            # Echo back
            response = {
                "type": "echo",
                "original": data,
                "timestamp": asyncio.get_event_loop().time()
            }
            await websocket.send(json.dumps(response))
            
    except websockets.exceptions.ConnectionClosed:
        print("Client disconnected")
    finally:
        websocket_clients.discard(websocket)

async def main():
    print("Starting minimal WebSocket server on ws://localhost:8765")
    
    # Start server
    async with websockets.serve(handler, "localhost", 8765):
        print("WebSocket server running!")
        await asyncio.Future()  # Run forever

if __name__ == "__main__":
    asyncio.run(main())
