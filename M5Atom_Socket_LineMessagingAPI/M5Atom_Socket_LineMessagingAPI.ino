/*
 * M5ATOM Socket Power Monitor example 
 * Copyright (c) 2025 @tomorrow56 All rights reserved.
 *  https://x.com/tomorrow56
 *  MIT License
 * Main Function:
 *  - Monitor socket power with ATOM Socket.
 *  - Toggle power on/off using the middle button.
 *  - Connect to Wi-Fi and send the device's IP address to the LINE Message.
 *  - Refer to README.md for details.
 */

#include <ElegantOTA.h>
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
char DEVICE_NAME[32]; // デバイス名を設定
// デバッグモード設定
#define debug true

/*
 * ambient account information
*/
#include <Ambient.h>
Ambient ambient;

uint16_t SendInterval = 1 * 60 * 1000; // メッセージ送信判定の間隔(60秒)
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
// LINEチャネルアクセストークンは以下から取得する
// https://developers.line.biz/ja/
// LINEチャネルアクセストークン (デフォルト値)
const char* accessToken = "<your access token>";

// NVSから読み込んだAPIキーを格納するグローバル変数
String storedApiKey;

// ライブラリインスタンス作成
ESP32LineMessenger line;

int Voltage, ActivePower = 0;
float Current = 0;

/*
 * ATOM Socket pin define
*/
#define RXD 22
#define RELAY 23

// シリアル通信用
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

// 追加する変数
uint32_t currentLowStartTime = 0; // 電流が閾値以下になった時刻
bool powerOffScheduled = false; // 電源オフがスケジュールされたか
uint16_t PowerOffDuration = 60; // 電源オフまでの時間（分単位、デフォルト60分）

// Ambient設定変数
bool ambientEnabled = false;
unsigned int ambientChannelId = 0;
char ambientWriteKey[33] = "";
bool autoOffEnabled = true; // Auto OFF機能の有効/無効設定（デフォルトは有効）

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

void onOTAStart() {
  // OTA開始時にシリアルにログを出力
  Serial.println("OTA update started!");
  // リレーをオフにして安全を確保
  RelayFlag = false;
  // シリアル通信をフラッシュして、すべての出力を確実に送信
  Serial.flush();
}

void onOTAProgress(size_t current, size_t final) {
  // Log every 1 second
  if (millis() - ota_progress_millis > 1000) {
    ota_progress_millis = millis();
    Serial.printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
  }
}

void onOTAEnd(bool success) {
  // OTA終了時にシリアルにログを出力
  if (success) {
    Serial.println("OTA update finished successfully!");
    // 成功時は3秒後にリセット
    delay(3000);
    ESP.restart();
  } else {
    Serial.println("There was an error during OTA update!");
    // 失敗時はリレー状態を元に戻す
    RelayFlag = true;  // または適切な初期状態に
  }
  
  // シリアル通信をフラッシュ
  Serial.flush();
}

void setup(){
  //M5::begin(SerialEnable = true, I2CEnable = true, DisplayEnable = false);
  M5.begin(true, false, true);
  M5.dis.drawpix(0, 0xe0ffff);
  ATOM.Init(AtomSerial, RELAY, RXD);

  // 設定値を読み込む
  loadSettings(); // NVSから設定値を読み込む
  loadDeviceSettings(); // DEVICE_NAMEをNVSから読み込む
  loadAmbientSettings(); // Ambient設定をNVSから読み込む
  storedApiKey = loadLineApiKey(); // LINE Access tokenをNVSから読み込む

  // アクセストークン設定
  if (storedApiKey.length() > 0){
    line.setAccessToken(storedApiKey.c_str());
  } else {
    line.setAccessToken(accessToken);
  }
  
  // デバッグモード設定
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

  // Webサーバーのエンドポイント設定
  server.on("/", handleRoot);
  server.on("/ip", handleIP);  // IPアドレス取得用エンドポイント
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
  server.on("/settings", handleSettings); // Settingページ
  server.on("/savesettings", HTTP_POST, handleSaveSettings); // Setting保存
  server.on("/getsettings", handleGetSettings); // Setting取得
  
  // CORS対応
  server.enableCORS(true);
  
  server.begin();
  Serial.println("HTTP server started");
  
  // ElegantOTAの設定
  ElegantOTA.setAuth("admin", "admin");  // ユーザー名とパスワードを設定
  
  // デバッグ用コールバックを設定
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);
  
  // ElegantOTAを開始
  ElegantOTA.begin(&server);
  
  // その他の初期化処理の前にOTAを優先
  delay(1000);  // 安定化のための遅延

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
          currentLowStartTime = millis(); // 電流が閾値以下になった時刻を記録
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
    } else if (powerOffScheduled && Current >= CurrentTH) {
      // Cancel Power OFF Schedule
      powerOffScheduled = false;
      Serial.println("Power off schedule cancelled: Current exceeded threshold.");
    }
  }

  // 物理ボタンでのON/OFF制御
  if (M5.Btn.isPressed()) {
    // ボタンが押された
    if (buttonPressStartTime == 0) {
      // ボタンが押された時間を記録
      buttonPressStartTime = millis();
      longPressTriggered = false;
    } else if (millis() - buttonPressStartTime > longPressDuration) {
      if (!longPressTriggered) {
      // longPressDuration(5秒)以上押されていたら長押しとしてAPモードに以降
        longPressTriggered = true;
        Serial.println("Long press detected. Starting WiFi Manager AP.");
        M5.dis.drawpix(0, 0x0000ff); // 青色でAPモード中を表示

        // メインのWebサーバーを停止してポートの競合を防ぐ
        server.stop();

        WiFiManager wifiManager;
        // Increased buffer size to 200 to accommodate full LINE API key length
        WiFiManagerParameter custom_line_api_key("lineapikey", "LINE Messaging API Key", "", 200);
        wifiManager.addParameter(&custom_line_api_key);
        wifiManager.setDebugOutput(true);
        wifiManager.setConfigPortalTimeout(180); // 3分でタイムアウト

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
    // ボタンが押されていない
    if (buttonPressStartTime > 0 && !longPressTriggered) {
      unsigned long pressDuration = millis() - buttonPressStartTime;
      // shortPressDuration(200ms)以上押されていたら短押しとして処理
      if (pressDuration >= shortPressDuration) {
        RelayFlag = !RelayFlag;
        // ボタンでRelayFlagが変更された場合、電源オフスケジュールをリセット
        powerOffScheduled = false;
        Serial.print("Button: Power ");
        Serial.println(RelayFlag ? "ON" : "OFF");
      } else {
        Serial.println("Button: Short press ignored (debounce)");
      }
    }
    // 状態をリセット
    buttonPressStartTime = 0;
  }

  // リレー制御とLED表示
  if(RelayFlag) {
    M5.dis.drawpix(0, 0xff0000);  // 赤色でON状態を表示
    ATOM.SetPowerOn();
  }else{
    ATOM.SetPowerOff();
    M5.dis.drawpix(0, 0x00ff00);  // 緑色でOFF状態を表示
  }
  
  M5.update();
  server.handleClient();
  ElegantOTA.loop();
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

/*** NVSに設定値を保存 ***/
void saveSettings(float currentTH, uint16_t powerOffDuration, bool autoOffEnabled) {
  preferences.begin("atom-socket", false);
  preferences.putFloat("current_th", currentTH);
  preferences.putUShort("power_off_dur", powerOffDuration);
  preferences.putBool("auto_off_en", autoOffEnabled);
  preferences.end();
  Serial.println("Settings saved to NVS.");
}

/*** NVSから設定値をロード ***/
void loadSettings() {
  preferences.begin("atom-socket", false);
  CurrentTH = preferences.getFloat("current_th", 0.3); // デフォルト値は0.3
  PowerOffDuration = preferences.getUShort("power_off_dur", 60); // デフォルト値は60
  autoOffEnabled = preferences.getBool("auto_off_en", true); // デフォルト値はtrue
  preferences.end();
  Serial.println("Settings loaded from NVS.");
  Serial.print("Loaded CurrentTH: "); Serial.println(CurrentTH);
  Serial.print("Loaded PowerOffDuration: "); Serial.println(PowerOffDuration);
  Serial.print("Loaded AutoOffEnabled: "); Serial.println(autoOffEnabled ? "true" : "false");
}

// Settingページ表示用ハンドラ
void handleSettings() {
  server.send(200, "text/html", settings_html);
}

// // Setting値保存用ハンドラ
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
    
    // 設定を保存
    saveSettings(newCurrentTH, newPowerOffDuration, newAutoOffEnabled);
    saveDeviceName(newDeviceName.c_str());
    saveAmbientSettings(newAmbientEnabled, newAmbientChannelId, newAmbientWriteKey.c_str());
    
    // グローバル変数を更新
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

// Setting値取得用ハンドラ
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

// NVSにDEVICE_NAMEを保存する関数
void saveDeviceName(const char* deviceName) {
  preferences.begin("atom-socket", false);
  preferences.putString("device_name", deviceName);
  preferences.end();
  Serial.println("DEVICE_NAME saved to NVS.");
}

// NVSからDEVICE_NAMEを読み込む関数
void loadDeviceSettings() {
  preferences.begin("atom-socket", false);
  String loadedName = preferences.getString("device_name", "M5Atom Socket"); // デフォルト値は"M5Atom Socket"
  loadedName.toCharArray(DEVICE_NAME, sizeof(DEVICE_NAME));
  preferences.end();
  Serial.print("Loaded DEVICE_NAME: "); Serial.println(DEVICE_NAME);
}

// NVSにAmbient設定を保存する関数
void saveAmbientSettings(bool enabled, unsigned int channelId, const char* writeKey) {
  preferences.begin("atom-socket", false);
  preferences.putBool("ambient_en", enabled);
  preferences.putUInt("ambient_ch", channelId);
  preferences.putString("ambient_key", writeKey);
  preferences.end();
  Serial.println("Ambient settings saved to NVS.");
}

// NVSからAmbient設定を読み込む関数
void loadAmbientSettings() {
  preferences.begin("atom-socket", false);
  ambientEnabled = preferences.getBool("ambient_en", false); // デフォルトは無効
  ambientChannelId = preferences.getUInt("ambient_ch", 0); // デフォルトは0
  String loadedKey = preferences.getString("ambient_key", "");
  loadedKey.toCharArray(ambientWriteKey, sizeof(ambientWriteKey));
  preferences.end();
  Serial.print("Loaded Ambient Enabled: "); Serial.println(ambientEnabled ? "true" : "false");
  Serial.print("Loaded Ambient Channel ID: "); Serial.println(ambientChannelId);
  Serial.print("Loaded Ambient Write Key: "); Serial.println(ambientWriteKey);
}
