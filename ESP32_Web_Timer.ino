#include <LittleFS.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include "my_littlefs_lib.h"
#include "file_handler.h" // New file for file handling functions
#include "config.h" // New file for configuration

WebServer server(80);
DNSServer dnsServer;

const int switchPin = 2; // GPIO2 on ESP32
bool timerRunning = false;

enum ResetState {
  NORMAL,
  READY,
  LOCKED
};
ResetState resetState = NORMAL;

void setup() {
  Serial.begin(115200);
  pinMode(switchPin, INPUT);
  
  // Initialize LittleFS
  if (!LittleFS.begin()) {
    Serial.println("Failed to mount file system");
    return;
  }

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Setup DNS server
  dnsServer.start(53, "*", WiFi.localIP());

  // Setup web server routes
  server.on("/file", HTTP_GET, []() {
    handleFileRequest(server.uri());
  });

  server.begin();
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
}

void handleStartStop() {
  if (resetState == NORMAL) {
    timerRunning = !timerRunning;
    server.send(200, "application/json", "{\"success\":true,\"running\":" + String(timerRunning ? "true" : "false") + "}");
    Serial.print("Timer: ");
    Serial.println(String(timerRunning ? "RUNNING" : "STOPPED"));
  } else {
    server.send(400, "application/json", "{\"success\":false,\"message\":\"Cannot start/stop timer\"}");
    Serial.println("Timer: CANNOT Start/Stop");
  }
}

void handleReset() {
  if (resetState == READY || resetState == NORMAL) {
    timerRunning = false;
    resetState = NORMAL;
    server.send(200, "application/json", "{\"success\":true}");
    Serial.println("Timer: RESET");
  } else {
    server.send(400, "application/json", "{\"success\":false,\"message\":\"Cannot reset timer\"}");
  }
}

void handleTimerStatus() {
  String json = "{\"running\":" + String(timerRunning ? "true" : "false") + ",";
  json += "\"startDisabled\":" + String((resetState != NORMAL) ? "true" : "false") + ",";
  json += "\"startLabel\":\"" + String((resetState != NORMAL) ? "Climber Pressed" : (timerRunning ? "Stop" : "Start")) + "\",";
  json += "\"resetState\":\"" + String(resetState == READY ? "READY" : (resetState == LOCKED ? "LOCKED" : "NORMAL")) + "\",";
  json += "\"resetLabel\":\"" + String(resetState == READY ? "Reset Ready" : (resetState == LOCKED ? "Reset Locked" : "Reset")) + "\"}";
  server.send(200, "application/json", json);
}

// Implement other functions like handleFileList, handleFileSave, handleFileLoad, and handleTimeSet as needed

#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include <LittleFS.h>
#include <WebServer.h>

extern WebServer server;

String getMimeType(const String& path) {
  if (path.endsWith(".html")) return "text/html";
  else if (path.endsWith(".css")) return "text/css";
  else if (path.endsWith(".js")) return "application/javascript";
  else if (path.endsWith(".png")) return "image/png";
  else if (path.endsWith(".svg")) return "image/svg+xml";
  else if (path.endsWith(".csv")) return "text/csv";
  else if (path.endsWith(".ico")) return "image/x-icon";
  return "text/plain";
}

void handleFileRequest(const String& path) {
  String contentType = getMimeType(path);
  Serial.print("File Request: ");
  Serial.println(path);
  if (LittleFS.exists(path)) {
    File file = LittleFS.open(path, "r");
    if (!file) {
      server.send(500, "text/plain", "Failed to open file");
      Serial.println("File Request: FAILED to open file");
      return;
    }
    server.streamFile(file, contentType);
    file.close();
    Serial.print(contentType);
    Serial.println(" --> File Request: SUCCESSFUL");
  } else {
    server.send(404, "text/plain", "File not found");
    Serial.println("File Request: FAILED");
  }
}

#endif // FILE_HANDLER_H

#ifndef CONFIG_H
#define CONFIG_H

const char* ssid = "Climbing_WiFi";
const char* password = "climbclimb";

#endif // CONFIG_H
