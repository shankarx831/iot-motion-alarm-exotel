

/*
 * IoT Motion Detection Alarm System - ESP8266 Code (Optimized)
 */

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

// ==================== CONFIGURATION ====================
const char* WIFI_SSID = "Shankar";          
const char* WIFI_PASSWORD = "shankar052";  

const char* BACKEND_URL = "https://iot-motion-alarm-exotel.onrender.com/api/motion/detect";
const char* JSON_PAYLOAD = "{\"source\":\"ESP8266\",\"sensor\":\"PIR\"}"; // Pre-allocated static payload
  
const int PIR_PIN = 13;           
const int LED_PIN = 2;            

const unsigned long MOTION_COOLDOWN = 5000;  

// ==================== GLOBAL VARIABLES ====================
unsigned long lastMotionTime = 0;
bool pirState = LOW;

// Optimization 1: Global secure client for SSL session reuse
WiFiClientSecure secureClient; 

// ==================== SETUP ====================

void setup() {
  Serial.begin(115200);
  delay(1000);  
  
  Serial.println("\n=================================");
  Serial.println("IoT Motion Alarm System Starting");
  Serial.println("=================================");
  
  pinMode(PIR_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH); // Active LOW: HIGH is OFF
  
  // Set up the secure client once
  secureClient.setInsecure();
  
  connectToWiFi();
  
  Serial.println("\nSetup complete! Monitoring for motion...");
  Serial.println("----------------------------------------\n");
}

// ==================== MAIN LOOP ====================

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected! Reconnecting...");
    connectToWiFi();
  }
  
  pirState = digitalRead(PIR_PIN);
  
  if (pirState == HIGH) {
    unsigned long currentTime = millis();
    
    if (currentTime - lastMotionTime >= MOTION_COOLDOWN) {
      Serial.println("\n>>> MOTION DETECTED! <<<");
      Serial.print("Timestamp: ");
      Serial.println(currentTime);
      
      digitalWrite(LED_PIN, LOW); // ON
      delay(100);
      digitalWrite(LED_PIN, HIGH); // OFF
      
      sendMotionAlert();
      
      lastMotionTime = currentTime;
    } 
  }
  
  // Reduced delay to 50ms - responsive enough while still feeding the watchdog
  delay(50);
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
  } else {
    Serial.println("\nFailed to connect to WiFi!");
  }
}

// ==================== HTTP FUNCTIONS ====================

void sendMotionAlert() {
  if (WiFi.status() != WL_CONNECTED) return;
  
  Serial.println("Sending motion alert to backend...");
  
  HTTPClient http;
  
  // Use the global secure client
  http.begin(secureClient, BACKEND_URL);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Connection", "keep-alive"); // Optimization 5: Request keep-alive
  http.setTimeout(8000);  // Reduced timeout slightly
  
  int httpResponseCode = http.POST(JSON_PAYLOAD);
  
  if (httpResponseCode > 0) {
    Serial.print("HTTP Response Code: ");
    Serial.println(httpResponseCode);
    
    // Optimization 3: Stack allocation for smaller footprint
    // Adjust size (256) based on expected JSON response size
    StaticJsonDocument<256> doc; 
    
    // Optimization 2: Parse directly from stream to avoid large String allocations
    DeserializationError error = deserializeJson(doc, http.getStream());
    
    if (!error) {
      bool callTriggered = doc["call_triggered"];
      const char* message = doc["message"]; 
      
      Serial.println("--- Backend Response ---");
      Serial.print("Message: ");
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
  
  http.end();
}