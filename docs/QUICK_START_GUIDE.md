# Mini SONiC Demo - Quick Start Guide

## 🚀 **Quick Start Instructions**

### **Step 1: Start the Server**
Open terminal and run:
```bash
cd d:\work\Projects\Mini_SONiC\mini_switch
python final_server.py
```

**Expected Output:**
```
Starting Final Mini SONiC Server...
HTTP API: http://localhost:8080
WebSocket: DISABLED (using HTTP polling fallback)
 * Running on http://127.0.0.1:8080
```

### **Step 2: Open the Web Demo**
Open browser and navigate to:
```
file:///d:/work/Projects/Mini_SONiC/docs/switch_animation_demo.html
```

### **Step 3: Verify Everything Works**

1. **Check Server Status**: Look for "Running on http://127.0.0.1:8080"
2. **Open Browser**: Firefox/Chrome/Edge should work
3. **Test Animation**: Click "Запустить" button
4. **Generate Packets**: Use packet generation controls
5. **Monitor Stats**: Watch real-time statistics

---

## 🎯 **What You Should See:**

### **Server Console:**
- ✅ Server startup messages
- ✅ HTTP API endpoints active
- ✅ CORS enabled for cross-origin requests

### **Web Browser:**
- ✅ Mini SONiC switch interface
- ✅ Interactive controls (start/stop/reset)
- ✅ Real-time packet animation
- ✅ Statistics dashboard
- ✅ Log messages showing server communication

### **Expected Log Messages:**
```
[INFO] Initializing Mini SONiC network with server connection...
[SUCCESS] Mini SONiC network initialized successfully
[INFO] Server monitoring started
[SUCCESS] Connected to Mini SONiC server
[INFO] Sending packet to server: 00:11:22:33:44:55 -> 66:55:44:33:22:11
[SUCCESS] Core Switch: L2 packet processed via server -> port flood
```

---

## 🔧 **Troubleshooting:**

### **If Server Doesn't Start:**
```bash
# Check if Python is installed
python --version

# Check required packages
pip install flask flask-cors

# Try different port
python final_server.py
# Then update client URL if needed
```

### **If Web Page Shows Errors:**
1. **Check server is running** (curl test)
2. **Clear browser cache** (Ctrl+F5)
3. **Check browser console** (F12)
4. **Try different browser**

### **If Packets Don't Process:**
1. **Check server logs** for errors
2. **Verify API endpoints** with curl
3. **Check CORS headers** in browser Network tab

---

## 📊 **Test Commands:**

### **Test Server Health:**
```bash
curl http://localhost:8080/api/health
```

### **Test Packet Processing:**
```bash
curl -X POST -H "Content-Type: application/json" \
-d '{"packetId":1,"srcMac":"00:11:22:33:44:55","dstMac":"66:55:44:33:22:11","type":"L2","ingressPort":"left-0","ingressSwitch":1}' \
http://localhost:8080/api/packet
```

### **Test Switch Status:**
```bash
curl http://localhost:8080/api/switch/1/status
```

---

## 🎉 **Success Indicators:**

### ✅ **Fully Working Demo Shows:**
- Server console with startup messages
- Web interface loads without errors
- "Запустить" button starts animation
- Packets move between switches
- Statistics update in real-time
- Log shows successful server communication

### 🌐 **Browser Compatibility:**
- ✅ **Firefox**: Full functionality
- ✅ **Chrome**: Full functionality  
- ✅ **Edge**: Full functionality
- ✅ **Safari**: Should work (CORS enabled)

---

## 📞 **Need Help?**

### **Common Issues:**
1. **"Server connection lost"** → Server not running
2. **"CORS error"** → Server CORS not configured
3. **"WebSocket failed"** → Normal, using HTTP fallback
4. **"Packet processing error"** → Check server logs

### **Solutions:**
1. **Restart server**: Stop Python process and restart
2. **Check ports**: Ensure 8080 is free
3. **Clear browser**: Hard refresh (Ctrl+F5)
4. **Check logs**: Look at server console output

---

## 🚀 **Ready to Go!**

Follow these steps and you'll have a fully functional Mini SONiC network switching demo with:
- Real-time packet processing
- Interactive web interface  
- Enterprise-grade architecture visualization
- Cross-browser compatibility
- Professional presentation capabilities

**Demo Status: PRODUCTION READY!** 🎊
