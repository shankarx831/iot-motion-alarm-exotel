# IoT Motion Alarm System (Exotel Edition)

A proactive IoT security solution built with Flask that triggers real-time automated phone call alerts via **Exotel API** upon motion detection. Featuring a premium web-based monitoring dashboard for remote management.

---

## 🚀 Key Features

*   📞 **Automated Voice Alerts:** Immediate phone call notifications when the PIR sensor detects movement.
*   🖥️ **Premium Dashboard:** Secure web interface to monitor alarm status, view historical event logs, and analyze activity patterns.
*   🔒 **Remote Control:** Arm or disarm the security system from anywhere via the dashboard.
*   ⚙️ **High Performance Backend:** Python Flask-based API with SQLite database persistence.
*   📱 **Responsive Design:** Fully mobile-responsive interface for security on the go.

---

## 🛠️ System Architecture

```text
+------------------+        WiFi         +-----------------------+
|   ESP32 + PIR    | --------------------> |    Flask Backend      |
|    (Hardware)    |   HTTP POST Request |    (Python/SQLite)    |
+------------------+   (Motion Alert)    +-----------+-----------+
                                                     |
                                                     |
                            +------------------------+------------------------+
                            |                        |                        |
                            v                        v                        v
                    +----------------+       +----------------+       +----------------+
                    | Admin Dashboard |       |   Exotel API   |       |   SQLite DB    |
                    | (HTML/CSS/JS)   |       |  (Phone Calls) |       | (Logs/Status)  |
                    +----------------+       +-------+--------+       +----------------+
                                                     |
                                                     v
                                             +------------------+
                                             |  Owner's Phone   |
                                             |  (Voice Alert)   |
                                             +------------------+
```

---

## 📋 Hardware Required

| Component | Specification | Description |
|-----------|--------------|-------------|
| **ESP32 DevKit** | 30-pin V1 | Handles WiFi and sensor logic |
| **PIR Sensor** | HC-SR501 | Detects infrared movement |
| **Jumper Wires** | F-F | Connectivity |

**Wiring:** 
- PIR VCC -> ESP32 3V3/VIN
- PIR GND -> ESP32 GND
- PIR OUT -> ESP32 GPIO 13

---

## ⚙️ Software Setup

### 1. Backend Configuration
1. **Ensure Python 3.9+** is installed.
2. Install dependencies:
   ```bash
   pip install -r requirements.txt
   ```
3. Setup Environment Variables:
   ```bash
   cp .env.template .env
   # Open .env and add your Exotel API Key, SID, Token, and App ID
   ```

### 2. Running the System (macOS/Linux)
We use port **5001** to avoid conflicts with macOS AirPlay services.
```bash
chmod +x run_app.sh
./run_app.sh
```
Access the dashboard at: `http://localhost:5001`

---

## 📞 Exotel Flow Configuration

To play the automated alert message, you must configure a "Flow" in your Exotel Dashboard:
1. Create a new **Applet/Flow** in the Exotel App Builder.
2. Add a **Greeting** module with your warning text (e.g., *"Motion detected at premises"*).
3. **Publish** the app and copy the **App ID**.
4. Paste the ID into your `.env` file as `EXOTEL_APP_ID`.

---

## 🔗 API Endpoints

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/` | GET | Serves the Web Dashboard |
| `/api/login` | POST | User authentication |
| `/api/alarm/status` | GET | Current system status |
| `/api/alarm/toggle` | POST | Arm/Disarm system |
| `/api/motion/detect`| POST | Triggered by ESP32 on motion |
| `/api/motion/logs`  | GET | Fetch event log history |

---

## 📂 Project Structure

```text
.
├── app.py              # Main Flask Backend & Static Server
├── app.js              # Frontend Logic & API Integration
├── index.html          # Dashbord UI (HTML5)
├── styles.css          # Dashboard Aesthetics (Vanilla CSS)
├── models.py           # SQLAlchemy Database Models
├── database.py         # DB Initialization Logic
├── iot_motion_alarm.ino # ESP32 Hardware Firmware
├── run_app.sh          # Execution Script (Port 5001)
├── requirements.txt    # Python Dependencies
├── .env.template       # Environment Configuration Template
└── .gitignore          # Git Exclusion Rules
```

---

## 🔓 Default Credentials

- **Username:** `admin`
- **Password:** `password123`

---

## 📜 License
MIT License - Developed for Educational/IoT Security research.
