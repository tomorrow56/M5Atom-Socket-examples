/*
  Description: 
  Use ATOM Socket to monitor the socket power, press the middle button of ATOM to switch the socket power on and off.
  Connect to Wi-Fi router and access IP address to wirelessly control the socket power and view the power voltage, current and power information.
  IP address of device is send to LINE Notify.
  The following article was used as a reference.
  https://qiita.com/mine820/items/53c2a833937f1186539f
*/

#include "M5Atom.h"
#include "AtomSocket.h"

#include "esp32web.h"
#include <WiFiClientSecure.h>

#include "index.h"

#define useOTA
char OTAHostPrefix[]= "ATOM-OTA-";
char OTAHostname[256];
char WiFiAPPrefix[] = "ATOM-SOCKET-";
char WiFiAPname[256];

WebServer server(80);
WiFiClientSecure client;

const char* host = "notify-api.line.me";
const char* token = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
IPAddress ipadr;

int Voltage, ActivePower = 0;
float Current = 0;

#define RXD 22
#define RELAY 23

ATOMSOCKET ATOM;

HardwareSerial AtomSerial(2);

bool RelayFlag = false;

uint16_t SerialInterval = 1000;
uint32_t updateTime = 0;       // time for next update

void handleRoot() {
  server.send(200, "text/html", html);
}

String DataCreate() {
    String RelayState = (RelayFlag)?"on":"off";
    String Data = "vol:<mark>"+String(Voltage)+"</mark>V#current:<mark>"+String(Current)+"</mark>A#power:<mark>"+String(ActivePower)+"</mark>W#state:<mark>"+RelayState+"</mark>";
    return Data;
}

void setup(){
  client.setInsecure();
	
  //M5::begin(SerialEnable = true, I2CEnable = true, DisplayEnable = false);
  M5.begin(true, false, true);
  M5.dis.drawpix(0, 0xe0ffff);
  ATOM.Init(AtomSerial, RELAY, RXD);

  // Wifi and OTA setup
  uint64_t Chipid = GetChipid();
  sprintf(WiFiAPname, "%s%04X", WiFiAPPrefix, (uint16_t)Chipid);

  WiFiMgrSetup(WiFiAPname);
  Serial.println("connected to router(^^)");
  Serial.print("AP SSID: ");
  Serial.println(WiFi.SSID());
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());  //IP address assigned to ATOM
  Serial.println("");

  ipadr = WiFi.localIP();
  String message = "Connected!\r\nSSID: " +
                   (String)WiFi.SSID() + "\r\nIP adrs: " +
                   (String)ipadr[0] + "." +
                   (String)ipadr[1] + "."+
                   (String)ipadr[2] + "."+
                   (String)ipadr[3];
  LINE_Notify(message);

  delay(1000);

  #ifdef useOTA
    sprintf(OTAHostname, "%s%04X", OTAHostPrefix, (uint16_t)Chipid);
    OTASetup(OTAHostname);
    if (MDNS.begin("ATOM-SOCKET")) {
      Serial.println("MDNS responder started");
    }
  #endif

  server.on("/", handleRoot);

  server.on("/on", []() {
    RelayFlag = true;
    server.send(200, "text/plain", DataCreate());
  });

  server.on("/off", []() {
    RelayFlag = false;
    server.send(200, "text/plain", DataCreate());
  });

  server.on("/data", []() {
    server.send(200, "text/plain", DataCreate());
  });

  server.begin();
  Serial.println("HTTP server started");

  updateTime = millis(); // Next update time
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
  if(M5.Btn.wasPressed()){
    RelayFlag = !RelayFlag;
  }
  if(RelayFlag) {
    M5.dis.drawpix(0, 0xff0000);
    ATOM.SetPowerOn();
  }else{
    ATOM.SetPowerOff();
    M5.dis.drawpix(0, 0x00ff00);
  }
  M5.update();
  server.handleClient();

  #ifdef useOTA
    ArduinoOTA.handle();
  #endif
}

/*
 * Send message to Line Notify
 */
void LINE_Notify(String message){
  if(!client.connect(host, 443)){
    Serial.println("Connection failed!");
    return;
  }else{
    Serial.println("Connected to " + String(host));
    String query = String("message=") + message;
    String request = String("") +
                 "POST /api/notify HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "Authorization: Bearer " + token + "\r\n" +
                 "Content-Length: " + String(query.length()) +  "\r\n" + 
                 "Content-Type: application/x-www-form-urlencoded\r\n\r\n" +
                  query + "\r\n";
    client.print(request);
    client.println("Connection: close");
    client.println();

    while (client.connected()) {
      String line = client.readStringUntil('\n');
      if (line == "\r") {
        Serial.println("headers received");
        break;
      }
    }
    // if there are incoming bytes available
    // from the server, read them and print them:
    while (client.available()) {
      char c = client.read();
      Serial.write(c);
    }

    client.stop();
    Serial.println("closing connection");
    Serial.println("");
  }
}
