#include <ElegantOTA.h>
#include "M5Atom.h"
#include "AtomSocket.h"
#include "esp32web.h"
#include <WiFiClientSecure.h>
#include "index.h"
#include <ESP32LineMessenger.h>
#include "WiFiManager.h"

/*
 * Description: Use ATOM Socket to monitor the socket power, press the middle button of ATOM to switch the socket power on and off.
 * Connect to Wi-Fi router and access IP address to wirelessly control the socket power and view the power voltage, current and power information.
 * Send IP address of device to LINE Notify.
 * The following article was used as a reference.
 * https://qiita.com/mine820/items/53c2a833937f1186539f
 * https://qiita.com/tomorrow56/items/2049eea1f68b9c1fd471
*/

/*
 * Device configuration
*/
//#define DEVICE_NAME "M5Atom Socket"  // デバイス名を設定
#define DEVICE_NAME "Fraxinus"
// デバッグモード設定
#define debug true
// If you want to use ambient,
//#define useAmb

/*
 * ambient account information
*/
#ifdef useAmb
  #include <Ambient.h>
  Ambient ambient;
  unsigned int channelId = 40780; // Ambient channel ID
  const char* writeKey = "ed2ee1286bfeed07"; // write key
#endif

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
 * LINE Messaging API information
*/

// 以下からLINEチャネルアクセストークンを取得する
// https://developers.line.biz/ja/
const char* accessToken = "<YOUR_LINE_MESSAGING_API_CHANNEL_ACCESS_TOKEN>"; // LINEチャネルアクセストークン

// ライブラリインスタンス作成
ESP32LineMessenger line;

int Voltage, ActivePower = 0;
float Current = 0;

/*
 * ATOM Socket pin define
*/
#define RXD 22
#define RELAY 23

ATOMSOCKET ATOM;
HardwareSerial AtomSerial(2);
bool RelayFlag = false;
uint16_t SerialInterval = 1000;
uint32_t updateTime = 0; // time for next update

/*
 * for Current Check
*/
float CurrentPre = 0;
float CurrentTH = 0.3;

/*
 * for Long press
*/
unsigned long buttonPressStartTime = 0;
const unsigned long longPressDuration = 5000; // 5 seconds
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
  
  // メモリを解放するために不要な処理を一時停止
  // ここに他の一時停止が必要な処理があれば追加
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

  // アクセストークン設定
  line.setAccessToken(accessToken);
  
  // デバッグモード設定（オプション）
  line.setDebug(debug);

  // Wifi and OTA setup
  uint64_t Chipid = GetChipid();
  sprintf(WiFiAPname, "%s%04X", WiFiAPPrefix, (uint16_t)Chipid);
  WiFiMgrSetup(WiFiAPname);
  Serial.println("connected to router(^^)");
  Serial.print("AP SSID: ");
  Serial.println(WiFi.SSID());
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP()); //IP address assigned to ATOM
  Serial.println("");

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
  
  // CORS対応
  server.enableCORS(true);
  
  server.begin();
  Serial.println("HTTP server started");
  
  // ElegantOTAの設定
  ElegantOTA.setAuth("admin", "admin");  // ユーザー名とパスワードを設定（セキュリティのため）
  
  // デバッグ用コールバックを設定
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);
  
  // ElegantOTAを開始
  ElegantOTA.begin(&server);
  
  // その他の初期化処理の前にOTAを優先
  delay(1000);  // 安定化のための遅延

  // Ambient initialization
#ifdef useAmb
  ambient.begin(channelId, writeKey, &client2);
  Serial.println("ambient begin...");
  Serial.print(" channelID:");
  Serial.println(channelId);
  Serial.println("");
  delay(500);
#endif

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
#ifdef useAmb
      Serial.println("Sending to ambient...");
      ambient.set(1, String(Voltage).c_str());
      ambient.set(2, String(Current).c_str());
      ambient.set(3, String(ActivePower).c_str());
      ambient.send();
#endif

      // Current Check
      if(Current < CurrentTH && CurrentPre >= CurrentTH){
        line.sendMessage("3D printing is finished.");
      }
      CurrentPre = Current;
    }
  }

  // 物理ボタンでのON/OFF制御 (短押し/長押し)
  if (M5.Btn.isPressed()) {
    if (buttonPressStartTime == 0) {
      // ボタンが押された瞬間
      buttonPressStartTime = millis();
      longPressTriggered = false;
    } else if (millis() - buttonPressStartTime > longPressDuration) {
      if (!longPressTriggered) {
        longPressTriggered = true;
        Serial.println("Long press detected. Starting WiFi Manager AP.");
        M5.dis.drawpix(0, 0x0000ff); // 青色でAPモード中を表示

        // メインのWebサーバーを停止してポートの競合を防ぐ
        server.stop();

        WiFiManager wifiManager;
        wifiManager.setDebugOutput(true);
        wifiManager.setConfigPortalTimeout(180); // 3分でタイムアウト

        if (wifiManager.startConfigPortal(WiFiAPname)) {
          Serial.println("WiFi settings configured successfully.");
        } else {
          Serial.println("Failed to configure WiFi or timed out.");
        }
        
        Serial.println("Restarting device...");
        delay(1000);
        ESP.restart();
      }
    }
  } else {
    // ボタンが離されている
    if (buttonPressStartTime > 0 && !longPressTriggered) {
      // 短押しとして処理
      RelayFlag = !RelayFlag;
      Serial.print("Button: Power ");
      Serial.println(RelayFlag ? "ON" : "OFF");
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

