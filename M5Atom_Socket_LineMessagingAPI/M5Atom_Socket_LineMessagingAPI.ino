/*
 * M5Atom Socket LINE Messaging API Controller
 * 
 * Copyright (c) 2025 @tomorrow56. All rights reserved.
 * 
 * Description: Use ATOM Socket to monitor the socket power, press the middle button of ATOM to switch the socket power on and off.
 * Connect to Wi-Fi router and access IP address to wirelessly control the socket power and view the power voltage, current and power information.
 * Send IP address of device to LINE Messaging API.
 * Please see README.md for reference.
 * 
 * Version: 11.0
 */


#include <ESP32FwUploader.h>
#include "M5Atom.h"
#include "AtomSocket.h"
#include "esp32web.h"
#include <WiFiClientSecure.h>
#include "index.h"
#include "settings.h"
#include <ESP32LineMessenger.h>
#include "WiFiManager.h"
#include <Preferences.h>

Preferences preferences;

/*
 * Device configuration
*/
char DEVICE_NAME[32]; // Set device name
// Debug mode setting
#define debug true

/*
 * ambient account information
*/
#include <Ambient.h>
Ambient ambient;

uint16_t SendInterval = 1 * 60 * 1000;
uint32_t updateSendTime = 0; // time for next update

/*
 * Wi-Fi setting
 */
char WiFiAPPrefix[] = "ATOM-SOCKET-";
char WiFiAPname[256];

WebServer server(80);
WiFiClientSecure client; // client for LINE Notify
WiFiClient client2; // client for ambient
IPAddress ipadr;

/*
 * Line Messaging setting
 */
// Get LINE channel access token from the following URL
// https://developers.line.biz/ja/
// LINE channel access token (default value)
const char* accessToken = "<Your Access Token>";

// Global variable to store API key loaded from NVS
String storedApiKey;

// Create library instance
ESP32LineMessenger line;

int Voltage, ActivePower = 0;
float Current = 0;

/*
 * ATOM Socket pin define
*/
#define RXD 22
#define RELAY 23

// For serial communication
HardwareSerial AtomSerial(1);

ATOMSOCKET ATOM;
bool RelayFlag = false;
uint16_t SerialInterval = 1000;
uint32_t updateTime = 0; // time for next update

/*
 * for Current Check
*/
float CurrentPre = 0;
float CurrentTH = 0.3;

// Additional variables
uint32_t currentLowStartTime = 0; // Time when current fell below threshold
bool powerOffScheduled = false; // Whether power off is scheduled
uint16_t PowerOffDuration = 60; // Time until power off (in minutes, default 60 minutes)

// Ambient configuration variables
bool ambientEnabled = false;
unsigned int ambientChannelId = 0;
char ambientWriteKey[33] = "";
bool autoOffEnabled = true; // Auto OFF function enable/disable setting (default is enabled)

/*
 * Button press timing
 */
unsigned long buttonPressStartTime = 0;
const unsigned long shortPressDuration = 200;    // 200ms minimum for short press
const unsigned long longPressDuration = 5000;    // 5 seconds for long press
bool longPressTriggered = false;

void handleRoot() {
  String htmlWithDeviceName = String(html);
  htmlWithDeviceName.replace("%DEVICE_NAME%", DEVICE_NAME);
  server.send(200, "text/html", htmlWithDeviceName);
}

void handleIP() {
  String ipString = String(ipadr[0]) + "." + String(ipadr[1]) + "." + String(ipadr[2]) + "." + String(ipadr[3]);
  server.send(200, "text/plain", ipString);
}

String DataCreate() {
  String RelayState = (RelayFlag)?"on":"off";
  String Data = "vol:<mark>"+String(Voltage)+"</mark>V#current:<mark>"+String(Current)+"</mark>A#power:<mark>"+String(ActivePower)+"</mark>W#state:<mark>"+RelayState+"</mark>";
  return Data;
}

unsigned long ota_progress_millis = 0;

void setup(){
  //M5::begin(SerialEnable = true, I2CEnable = true, DisplayEnable = false);
  M5.begin(true, false, true);
  M5.dis.drawpix(0, 0xe0ffff);
  ATOM.Init(AtomSerial, RELAY, RXD);

  // Load configuration values
  loadSettings(); // Load settings from NVS
  loadDeviceSettings(); // Load DEVICE_NAME from NVS
  loadAmbientSettings(); // Load Ambient settings from NVS
  storedApiKey = loadLineApiKey(); // Load LINE Access token from NVS

  // Set access token
  if (storedApiKey.length() > 0){
    line.setAccessToken(storedApiKey.c_str());
  } else {
    line.setAccessToken(accessToken);
  }
  
  // Set debug mode
  line.setDebug(debug);

  // Wifi and OTA setup
  uint64_t Chipid = GetChipid();
  sprintf(WiFiAPname, "%s%04X", WiFiAPPrefix, (uint16_t)Chipid);
  WiFiMgrSetup(WiFiAPname);

  ipadr = WiFi.localIP();
  String message = "Connected!\r\nDevice: " + String(DEVICE_NAME)
                   + "\r\nSSID:" + (String)WiFi.SSID()
                   + "\r\nIP adrs: "
                   + (String)ipadr[0] + "."
                   + (String)ipadr[1] + "."
                   + (String)ipadr[2] + "."
                   + (String)ipadr[3];
  line.sendMessage(message.c_str());
  delay(1000);

  // Web server endpoint configuration
  server.on("/", handleRoot);
  server.on("/ip", handleIP);  // Endpoint for getting IP address
  server.on("/on", []() {
    RelayFlag = true;
    Serial.println("Web: Power ON");
    server.send(200, "text/plain", DataCreate());
  });
  server.on("/off", []() {
    RelayFlag = false;
    Serial.println("Web: Power OFF");
    server.send(200, "text/plain", DataCreate());
  });
  server.on("/data", []() {
    server.send(200, "text/plain", DataCreate());
  });
  server.on("/apikey", handleApiKey);
  server.on("/clearapikey", handleClearApiKey);
  server.on("/settings", handleSettings); // Settings page
  server.on("/savesettings", HTTP_POST, handleSaveSettings); // Save settings
  server.on("/getsettings", handleGetSettings); // Get settings
  
  // CORS support
  server.enableCORS(true);
  
  server.begin();
  Serial.println("HTTP server started");
  
   // Prioritize OTA before other initialization processes
  ESP32FwUploader.begin(&server);
  ESP32FwUploader.setDebug(true);
  ESP32FwUploader.setDarkMode(true);  // Enable dark mode
  ESP32FwUploader.onStart([]() {
    Serial.println("OTA update started!");
    RelayFlag = false;
    Serial.flush();
  });

  ESP32FwUploader.onProgress([](size_t current, size_t total) {
    Serial.printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, total);
  });

  ESP32FwUploader.onEnd([](bool success) {
    if (success) {
      Serial.println("OTA update finished successfully!");
      delay(3000);
      ESP.restart();
    } else {
      Serial.println("There was an error during OTA update!");
      RelayFlag = true;
    }
    Serial.flush();
  });
  
  ESP32FwUploader.onError([](ESP32Fw_Error error, const String& message) {
    Serial.printf("OTA Error: %d, Message: %s\n", error, message.c_str());
  });
  
  delay(1000);  // Delay for stabilization

  // Ambient initialization
  if (ambientEnabled && ambientChannelId > 0 && strlen(ambientWriteKey) > 0) {
    ambient.begin(ambientChannelId, ambientWriteKey, &client2);
    Serial.println("ambient begin...");
    Serial.print(" channelID:");
    Serial.println(ambientChannelId);
    Serial.println("");
    delay(500);
  }

  updateTime = millis(); // Next update time
  updateSendTime = millis(); // Next send time
}

void loop(){
  ATOM.SerialReadLoop();
  if (updateTime <= millis()) {
    updateTime = millis() + SerialInterval; // Update interval
    if(ATOM.SerialRead == 1){
      Voltage = ATOM.GetVol();
      Current = ATOM.GetCurrent();
      ActivePower = ATOM.GetActivePower();
      Serial.print("Voltage: ");
      Serial.print(Voltage);
      Serial.println(" V");
      Serial.print("Current: ");
      Serial.print(Current);
      Serial.println(" A");
      Serial.print("ActivePower: ");
      Serial.print(ActivePower);
      Serial.println(" W");
      Serial.println("");
    }
  }

  if (updateSendTime <= millis()) {
    updateSendTime = millis() + SendInterval; // Send interval
    
    if(RelayFlag) {
      if (ambientEnabled && ambientChannelId > 0 && strlen(ambientWriteKey) > 0) {
        Serial.println("Sending to ambient...");
        ambient.set(1, String(Voltage).c_str());
        ambient.set(2, String(Current).c_str());
        ambient.set(3, String(ActivePower).c_str());
        ambient.send();
      }

      // Checking print end conditions
      if(Current < CurrentTH && CurrentPre >= CurrentTH){
        Serial.println("*** LINE Message is triggered ***");
        String message = "[" + String(DEVICE_NAME) + "] The work is finished.";
        if (autoOffEnabled) {
          message += "\r\n Auto OFF in " + String(PowerOffDuration) + "min.";
          currentLowStartTime = millis(); // Record time when current fell below threshold
          powerOffScheduled = true; // Auto OFF Schedule
        } else {
          message += "\r\n Auto OFF disabled.";
        }
        Serial.print("Sending LINE message: "); Serial.println(message);
        bool result = line.sendMessage(message.c_str());
        delay(1000);
        if(result){
          Serial.println("LINE send result: SUCCESS");
        }else{
          Serial.println("LINE send result: FAILED");
        }
      }
      // Update Current Previous
      CurrentPre = Current;
    }
  }

  // Auto OFF Function
  if (autoOffEnabled && powerOffScheduled && Current < CurrentTH) {
    if (millis() >= currentLowStartTime + (unsigned long)PowerOffDuration * 60 * 1000) {
      RelayFlag = false; // Power OFF
      powerOffScheduled = false; // Reset Schedule
      
      Serial.println("=== Auto Power OFF Notification ===");
      String powerOffMessage = "[" + String(DEVICE_NAME) + "] Auto Power OFF.";
      Serial.print("Sending LINE message: "); Serial.println(powerOffMessage);
      bool result = line.sendMessage(powerOffMessage.c_str());
      delay(1000);
      if(result){
        Serial.println("LINE send result: SUCCESS");
      }else{
        Serial.println("LINE send result: FAILED");
      }
    } 
  } else if (autoOffEnabled && powerOffScheduled && Current >= CurrentTH) {
    // If current exceeds threshold, cancel power off schedule and reset counter
    powerOffScheduled = false;
    currentLowStartTime = 0; // Reset counter
    Serial.println("Power off schedule cancelled: Current exceeded threshold. Counter reset.");
  }

  // Physical button ON/OFF control
  if (M5.Btn.isPressed()) {
    // Button is pressed
    if (buttonPressStartTime == 0) {
      // Record button press time
      buttonPressStartTime = millis();
      longPressTriggered = false;
    } else if (millis() - buttonPressStartTime > longPressDuration) {
      if (!longPressTriggered) {
      // If pressed for longPressDuration (5 seconds) or more, treat as long press and switch to AP mode
        longPressTriggered = true;
        Serial.println("Long press detected. Starting WiFi Manager AP.");
        M5.dis.drawpix(0, 0x0000ff); // Display AP mode in blue

        // Stop main web server to prevent port conflicts
        server.stop();

        WiFiManager wifiManager;
        // Increased buffer size to 200 to accommodate full LINE API key length
        WiFiManagerParameter custom_line_api_key("lineapikey", "LINE Messaging API Key", "", 200);
        wifiManager.addParameter(&custom_line_api_key);
        wifiManager.setDebugOutput(true);
        wifiManager.setConfigPortalTimeout(180); // 3 minute timeout

        if (wifiManager.startConfigPortal(WiFiAPname)) {
          Serial.println("WiFi settings configured successfully.");
          // Retrieve the custom parameter value
          String lineApiKey = custom_line_api_key.getValue();
          if (lineApiKey.length() > 0) {
            saveLineApiKey(lineApiKey);
            Serial.print("LINE API Key: ");
            Serial.println(lineApiKey);
          } else {
            Serial.println("No LINE API Key entered.");
          }
        } else {
          Serial.println("Failed to configure WiFi or timed out.");
        }
        
        Serial.println("Restarting device...");
        delay(1000);
        ESP.restart();
      }
    }
  } else {
    // Button is not pressed
    if (buttonPressStartTime > 0 && !longPressTriggered) {
      unsigned long pressDuration = millis() - buttonPressStartTime;
      // If pressed for shortPressDuration (200ms) or more, treat as short press
      if (pressDuration >= shortPressDuration) {
        RelayFlag = !RelayFlag;
        // If RelayFlag is changed by button, reset power off schedule
        powerOffScheduled = false;
        Serial.print("Button: Power ");
        Serial.println(RelayFlag ? "ON" : "OFF");
      } else {
        Serial.println("Button: Short press ignored (debounce)");
      }
    }
    // Reset state
    buttonPressStartTime = 0;
  }

  // Relay control and LED display
  if(RelayFlag) {
    M5.dis.drawpix(0, 0xff0000);  // Display ON state in red
    ATOM.SetPowerOn();
  }else{
    ATOM.SetPowerOff();
    M5.dis.drawpix(0, 0x00ff00);  // Display OFF state in green
  }
  
  M5.update();
  server.handleClient();
  ESP32FwUploader.loop();

}

/*** LINE API Key management***/

void saveLineApiKey(String apiKey) {
  preferences.begin("line-api", false);
  preferences.putString("api_key", apiKey);
  preferences.end();
  Serial.println("LINE API Key saved to NVS.");
}

String loadLineApiKey() {
  preferences.begin("line-api", false);
  String apiKey = preferences.getString("api_key", "");
  preferences.end();
  return apiKey;
}

void clearLineApiKey() {
  preferences.begin("line-api", false);
  preferences.remove("api_key");
  preferences.end();
  Serial.println("LINE API Key cleared from NVS.");
}

void handleApiKey() {
  String apiKey = loadLineApiKey();
  String response = "";
  if (apiKey.length() > 0) {
    response = "LINE API Key: " + apiKey + "<br>";
    response += "<a href=\"/clearapikey\">Clear LINE API Key</a>";
  } else {
    response = "No LINE API Key saved.<br>";
  }
  server.send(200, "text/html", response);
}

void handleClearApiKey() {
  clearLineApiKey();
  server.send(200, "text/plain", "LINE API Key cleared. Please restart the device.");
  delay(1000);
  ESP.restart();
}

/*** Save settings to NVS ***/
void saveSettings(float currentTH, uint16_t powerOffDuration, bool autoOffEnabled) {
  preferences.begin("atom-socket", false);
  preferences.putFloat("current_th", currentTH);
  preferences.putUShort("power_off_dur", powerOffDuration);
  preferences.putBool("auto_off_en", autoOffEnabled);
  preferences.end();
  Serial.println("Settings saved to NVS.");
}

/*** Load settings from NVS ***/
void loadSettings() {
  preferences.begin("atom-socket", false);
  CurrentTH = preferences.getFloat("current_th", 0.3); // Default value is 0.3
  PowerOffDuration = preferences.getUShort("power_off_dur", 60); // Default value is 60
  autoOffEnabled = preferences.getBool("auto_off_en", true); // Default value is true
  preferences.end();
  Serial.println("Settings loaded from NVS.");
  Serial.print("Loaded CurrentTH: "); Serial.println(CurrentTH);
  Serial.print("Loaded PowerOffDuration: "); Serial.println(PowerOffDuration);
  Serial.print("Loaded AutoOffEnabled: "); Serial.println(autoOffEnabled ? "true" : "false");
}

// Handler for displaying Settings page
void handleSettings() {
  server.send(200, "text/html", settings_html);
}

// Handler for saving setting values
void handleSaveSettings() {
  if (server.hasArg("deviceName") && server.hasArg("currentTH") && server.hasArg("powerOffDuration") && 
      server.hasArg("autoOffEnabled") && server.hasArg("ambientEnabled") && server.hasArg("ambientChannelId") && server.hasArg("ambientWriteKey")) {
    
    String newDeviceName = server.arg("deviceName");
    float newCurrentTH = server.arg("currentTH").toFloat();
    uint16_t newPowerOffDuration = server.arg("powerOffDuration").toInt();
    bool newAutoOffEnabled = (server.arg("autoOffEnabled") == "true");
    bool newAmbientEnabled = (server.arg("ambientEnabled") == "true");
    unsigned int newAmbientChannelId = server.arg("ambientChannelId").toInt();
    String newAmbientWriteKey = server.arg("ambientWriteKey");
    
    // Save settings
    saveSettings(newCurrentTH, newPowerOffDuration, newAutoOffEnabled);
    saveDeviceName(newDeviceName.c_str());
    saveAmbientSettings(newAmbientEnabled, newAmbientChannelId, newAmbientWriteKey.c_str());
    
    // Update global variables
    newDeviceName.toCharArray(DEVICE_NAME, sizeof(DEVICE_NAME));
    CurrentTH = newCurrentTH;
    PowerOffDuration = newPowerOffDuration;
    autoOffEnabled = newAutoOffEnabled;
    ambientEnabled = newAmbientEnabled;
    ambientChannelId = newAmbientChannelId;
    newAmbientWriteKey.toCharArray(ambientWriteKey, sizeof(ambientWriteKey));
    
    server.send(200, "text/plain", "Settings saved");
  } else {
    server.send(400, "text/plain", "Invalid arguments");
  }
}

// Handler for getting setting values
void handleGetSettings() {
  String json = "{\"deviceName\":\"" + String(DEVICE_NAME) + 
                "\", \"currentTH\":" + String(CurrentTH) + 
                ", \"powerOffDuration\": " + String(PowerOffDuration) + 
                ", \"autoOffEnabled\": " + (autoOffEnabled ? "true" : "false") +
                ", \"ambientEnabled\": " + (ambientEnabled ? "true" : "false") +
                ", \"ambientChannelId\": " + String(ambientChannelId) +
                ", \"ambientWriteKey\": \"" + String(ambientWriteKey) + "\"}";
  server.send(200, "application/json", json);
}

// Function to save DEVICE_NAME to NVS
void saveDeviceName(const char* deviceName) {
  preferences.begin("atom-socket", false);
  preferences.putString("device_name", deviceName);
  preferences.end();
  Serial.println("DEVICE_NAME saved to NVS.");
}

// Function to load DEVICE_NAME from NVS
void loadDeviceSettings() {
  preferences.begin("atom-socket", false);
  String loadedName = preferences.getString("device_name", "M5Atom Socket"); // Default value is "M5Atom Socket"
  loadedName.toCharArray(DEVICE_NAME, sizeof(DEVICE_NAME));
  preferences.end();
  Serial.print("Loaded DEVICE_NAME: "); Serial.println(DEVICE_NAME);
}

// Function to save Ambient settings to NVS
void saveAmbientSettings(bool enabled, unsigned int channelId, const char* writeKey) {
  preferences.begin("atom-socket", false);
  preferences.putBool("ambient_en", enabled);
  preferences.putUInt("ambient_ch", channelId);
  preferences.putString("ambient_key", writeKey);
  preferences.end();
  Serial.println("Ambient settings saved to NVS.");
}

// Function to load Ambient settings from NVS
void loadAmbientSettings() {
  preferences.begin("atom-socket", false);
  ambientEnabled = preferences.getBool("ambient_en", false); // Default is disabled
  ambientChannelId = preferences.getUInt("ambient_ch", 0); // Default is 0
  String loadedKey = preferences.getString("ambient_key", "");
  loadedKey.toCharArray(ambientWriteKey, sizeof(ambientWriteKey));
  preferences.end();
  Serial.print("Loaded Ambient Enabled: "); Serial.println(ambientEnabled ? "true" : "false");
  Serial.print("Loaded Ambient Channel ID: "); Serial.println(ambientChannelId);
  Serial.print("Loaded Ambient Write Key: "); Serial.println(ambientWriteKey);
}
