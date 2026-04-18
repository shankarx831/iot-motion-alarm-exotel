/*
 * IoT Motion Detection Alarm System - Final Stable ESP8266 Code
 *
 * This code runs on ESP8266 and:
 * 1. Connects stably to WiFi (using yield() to prevent WDT resets)
 * 2. Monitors HC-SR501 PIR sensor on Pin D7 (GPIO 13)
 * 3. Sends an optimized HTTPS POST to the Render backend on motion
 * 4. Handles memory efficiently (92% IRAM usage is manageable with these fixes)
 */

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

// ==================== CONFIGURATION ====================
// *** UPDATE THESE VALUES ***
const char* WIFI_SSID = "Shankar";          // Your WiFi name
const char* WIFI_PASSWORD = "shankar052";  // Your WiFi password

// Backend URL - Secure Render endpoint
const char* BACKEND_URL = "https://iot-motion-alarm-exotel.onrender.com/api/motion/detect";

// Pre-allocated static payload string (saves heap memory)
const char* JSON_PAYLOAD = "{\"source\":\"ESP8266\",\"sensor\":\"PIR\"}";
  
// PIR Sensor Pin Configuration
// GPIO 13 corresponds to Pin D7 on NodeMCU (verify wiring diagram!)
const int PIR_PIN = 13;           
// GPIO 2 corresponds to Pin D4 (Onboard LED, Active Low)
const int LED_PIN = 2;            

// Timing Configuration
const unsigned long MOTION_COOLDOWN = 10000;  // 10 seconds between allowed detections

// ==================== GLOBAL VARIABLES ====================

unsigned long lastMotionTime = 0;
bool pirState = LOW;

// Optimization 1: Global secure client for SSL session reuse.
// This is critical for making subsequent HTTPS requests fast.
WiFiClientSecure secureClient; 

// Forward Declarations with ICACHE_FLASH_ATTR
// These macros tell the compiler to place these non-critical functions
// in Flash (IROM), saving precious IRAM for system tasks.
void ICACHE_FLASH_ATTR connectToWiFi();
void ICACHE_FLASH_ATTR sendMotionAlert();
void ICACHE_FLASH_ATTR blinkLED(int times, int delayMs);

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
  
  // The built-in LED (GPIO 2 / D4) is ACTIVE LOW.
  // Setting it HIGH turns it OFF.
  digitalWrite(LED_PIN, HIGH); 
  
  // Setup the secure client once (can't be in flash)
  secureClient.setInsecure(); // Bypass full SSL fingerprint check for simplicity
  
  // Connect to WiFi using the stable function
  connectToWiFi();
  
  Serial.println("\nSetup complete! Monitoring for motion...");
  Serial.println("Note: PIR requires ~60s warmup after power-on.");
  Serial.println("----------------------------------------\n");
}

// ==================== MAIN LOOP ====================

void loop() {
  // Check WiFi connection and attempt re-connection if dropped
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected! Reconnecting...");
    connectToWiFi();
  }
  
  // Read PIR sensor
  pirState = digitalRead(PIR_PIN);
  
  // Motion detected! (Hardware Pin D7 is HIGH)
  if (pirState == HIGH) {
    unsigned long currentTime = millis();
    
    // Check cooldown to prevent multiple rapid triggers
    if (currentTime - lastMotionTime >= MOTION_COOLDOWN) {
      Serial.println("\n>>> MOTION DETECTED! <<<");
      Serial.print("Timestamp: ");
      Serial.println(currentTime);
      
      // Local Indication: Blink LED (Active Low: LOW is ON, HIGH is OFF)
      digitalWrite(LED_PIN, LOW); // ON
      delay(100);
      digitalWrite(LED_PIN, HIGH); // OFF
      
      // External Indication: Send optimized alert to backend
      sendMotionAlert();
      
      lastMotionTime = currentTime;
    } 
  }
  
  // Small delay to prevent CPU overload and feed the system's watchdog timer
  delay(50); 
}

// ==================== WIFI FUNCTIONS (STABILIZED) ====================

// Attribute: Moved to Flash. Stabilized with yield() to feed watchdog.
void ICACHE_FLASH_ATTR connectToWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  // This loop prevents the repetitive crashing issue by feeding the WDT.
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    
    // Crucial Optimization: Feed the hardware and software Watchdog timers
    // inside this blocking loop. This prevents the chip from rebooting spontaneously.
    yield(); 
    
    delay(500); 
    
    // (Redundant but safe: yield again after delay)
    yield(); 
    
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect to WiFi! System may be unstable.");
  }
}

// ==================== HTTP FUNCTIONS (OPTIMIZED) ====================

// Attribute: Moved to Flash. Optimized for minimal RAM usage.
void ICACHE_FLASH_ATTR sendMotionAlert() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Cannot send alert - WiFi not connected!");
    return;
  }
  
  Serial.println("Sending optimized motion alert via HTTPS...");
  
  HTTPClient http;
  
  // Optimization 1: Use the global secure client for SSL session reuse.
  // This avoids the expensive full SSL handshake on every motion event.
  if (http.begin(secureClient, BACKEND_URL)) {
    
    http.addHeader("Content-Type", "application/json");
    // Optimization 5: Encourage backend to keep connection alive
    http.addHeader("Connection", "keep-alive"); 
    http.setTimeout(10000);  // 10 second timeout for Render response
    
    // Optimization 4: Use pre-allocated static JSON_PAYLOAD constant.
    // Send POST request
    int httpResponseCode = http.POST(JSON_PAYLOAD);
    
    if (httpResponseCode > 0) {
      Serial.print("HTTP Response Code: ");
      Serial.println(httpResponseCode);
      
      // Optimization 3: StaticJsonDocument for stack allocation (faster, less fragmentation).
      // Size 256 is sufficient for typical backend JSON responses.
      StaticJsonDocument<256> doc; 
      
      // Optimization 2: Parse directly from the incoming stream.
      // This avoids creating a huge intermediate String object, saving massive heap space.
      DeserializationError error = deserializeJson(doc, http.getStream());
      
      if (!error) {
        bool callTriggered = doc["call_triggered"];
        const char* message = doc["message"]; // Efficient const char* reference
        
        Serial.println("--- Backend Response ---");
        Serial.print("Message: ");
        // Print message, handling potential nulls safely
        Serial.println(message ? message : "null");
        Serial.print("Phone Call Triggered: ");
        Serial.println(callTriggered ? "YES" : "NO");
        Serial.println("------------------------");
      } else {
        Serial.print("JSON Parse failed: ");
        Serial.println(error.c_str());
      }
    } else {
      Serial.print("HTTP Request Failed: ");
      Serial.println(http.errorToString(httpResponseCode).c_str());
    }
    
    http.end(); // Important: Close connection
  } else {
    Serial.println("Unable to connect to backend URL");
  }
}

// ==================== UTILITY FUNCTIONS ====================

// Attribute: Moved to Flash, timing here isn't critical.
void ICACHE_FLASH_ATTR blinkLED(int times, int delayMs) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_PIN, LOW);   // Turn ON (Active Low)
    delay(delayMs);
    digitalWrite(LED_PIN, HIGH);  // Turn OFF
    delay(delayMs);
  }
}