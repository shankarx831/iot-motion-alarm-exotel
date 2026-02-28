/*
 * IoT Motion Detection Alarm System - ESP32 Code
 * 
 * This code runs on ESP32 and:
 * 1. Connects to WiFi network
 * 2. Monitors PIR sensor for motion
 * 3. Sends HTTP POST to Flask backend when motion detected
 * 4. Handles WiFi disconnections gracefully
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// ==================== CONFIGURATION ====================
// Update these values for your network and backend

const char* WIFI_SSID = "YOUR_WIFI_SSID";          // Your WiFi name
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";  // Your WiFi password

// Backend URL - Update after deploying to Heroku/Render
// Example: "https://your-app-name.herokuapp.com/api/motion/detect"
const char* BACKEND_URL = "https://your-backend-url/api/motion/detect";
  
// PIR Sensor Pin Configuration
const int PIR_PIN = 13;           // GPIO 13 (D13 on most boards)
const int LED_PIN = 2;            // Built-in LED for status indication

// Timing Configuration
const unsigned long MOTION_COOLDOWN = 5000;  // 5 seconds between detections
const unsigned long WIFI_RETRY_INTERVAL = 10000;  // 10 seconds between WiFi retries

// ==================== GLOBAL VARIABLES ====================

unsigned long lastMotionTime = 0;
bool pirState = LOW;
bool motionDetected = false;

// ==================== SETUP ====================

void setup() {
  Serial.begin(115200);
  delay(1000);  // Give time for serial monitor to open
  
  Serial.println("\n=================================");
  Serial.println("IoT Motion Alarm System Starting");
  Serial.println("=================================");
  
  // Configure pins
  pinMode(PIR_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  // Connect to WiFi
  connectToWiFi();
  
  Serial.println("\nSetup complete! Monitoring for motion...");
  Serial.println("----------------------------------------\n");
}

// ==================== MAIN LOOP ====================

void loop() {
  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected! Reconnecting...");
    connectToWiFi();
  }
  
  // Read PIR sensor
  pirState = digitalRead(PIR_PIN);
  
  // Motion detected!
  if (pirState == HIGH) {
    unsigned long currentTime = millis();
    
    // Check cooldown to prevent multiple rapid triggers
    if (currentTime - lastMotionTime >= MOTION_COOLDOWN) {
      Serial.println("\n>>> MOTION DETECTED! <<<");
      Serial.print("Timestamp: ");
      Serial.println(currentTime);
      
      // Blink LED to indicate motion
      digitalWrite(LED_PIN, HIGH);
      delay(100);
      digitalWrite(LED_PIN, LOW);
      
      // Send alert to backend
      sendMotionAlert();
      
      lastMotionTime = currentTime;
    } else {
      Serial.println("Motion detected, but in cooldown period...");
    }
  }
  
  // Small delay to prevent CPU overload
  delay(100);
}

// ==================== WIFI FUNCTIONS ====================

void connectToWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Signal Strength (RSSI): ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  } else {
    Serial.println("\nFailed to connect to WiFi!");
    Serial.println("Check your credentials and try again.");
  }
}

// ==================== HTTP FUNCTIONS ====================

void sendMotionAlert() {
  // Check WiFi before sending
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Cannot send alert - WiFi not connected!");
    return;
  }
  
  Serial.println("Sending motion alert to backend...");
  
  HTTPClient http;
  
  // Begin HTTP POST request
  http.begin(BACKEND_URL);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(10000);  // 10 second timeout
  
  // Create JSON payload (optional - backend doesn't require body)
  String payload = "{\"source\":\"ESP32\",\"sensor\":\"PIR\"}";
  
  // Send POST request
  int httpResponseCode = http.POST(payload);
  
  // Handle response
  if (httpResponseCode > 0) {
    Serial.print("HTTP Response Code: ");
    Serial.println(httpResponseCode);
    
    String response = http.getString();
    Serial.print("Response: ");
    Serial.println(response);
    
    // Parse JSON response
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, response);
    
    if (!error) {
      bool callTriggered = doc["call_triggered"];
      String message = doc["message"];
      
      Serial.println("--- Backend Response ---");
      Serial.print("Message: ");
      Serial.println(message);
      Serial.print("Phone Call Triggered: ");
      Serial.println(callTriggered ? "YES" : "NO");
      Serial.println("------------------------");
    }
  } else {
    Serial.print("HTTP Request Failed: ");
    Serial.println(http.errorToString(httpResponseCode));
  }
  
  http.end();
}

// ==================== UTILITY FUNCTIONS ====================

void blinkLED(int times, int delayMs) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(delayMs);
    digitalWrite(LED_PIN, LOW);
    delay(delayMs);
  }
}
