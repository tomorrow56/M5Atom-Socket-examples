/*
    Description: 
    Use ATOM Socket to monitor the socket power, press the middle button of ATOM to switch the socket power on and off.
    Connect to the SoftAP hotspot of the device and access 192.168.4.1 to wirelessly control the socket power and view the power voltage, current and power information.
*/

#include "M5Atom.h"
#include "AtomSocket.h"

#include "esp32web.h"

#include "index.h"

//#define usesoftAP

#ifdef usesoftAP
  const char* ssid = "ATOM-SOCKET";
  const char* password = "12345678";
#else
  #define useOTA
  char OTAHostPrefix[]= "ATOM-OTA-";
  char OTAHostname[256];
  char WiFiAPPrefix[] = "ATOM-SOCKET-";
  char WiFiAPname[256];
#endif


int Voltage, ActivePower = 0;
float Current = 0;

#define RXD 22
#define RELAY 23

ATOMSOCKET ATOM;

HardwareSerial AtomSerial(2);

bool RelayFlag = false;

uint16_t SerialInterval = 1000;
uint32_t updateTime = 0;       // time for next update

WebServer server(80);

void handleRoot() {
  server.send(200, "text/html", html);
}

String DataCreate() {
    String RelayState = (RelayFlag)?"on":"off";
    String Data = "vol:<mark>"+String(Voltage)+"</mark>V#current:<mark>"+String(Current)+"</mark>A#power:<mark>"+String(ActivePower)+"</mark>W#state:<mark>"+RelayState+"</mark>";
    return Data;
}

void setup(){
  //M5::begin(SerialEnable = true, I2CEnable = true, DisplayEnable = false);
  M5.begin(true, false, true);
  M5.dis.drawpix(0, 0xe0ffff);
  ATOM.Init(AtomSerial, RELAY, RXD);

  // Wifi and OTA setup
  uint64_t Chipid = GetChipid();
  sprintf(WiFiAPname, "%s%04X", WiFiAPPrefix, (uint16_t)Chipid);
  #ifdef usesoftAP
    WiFi.softAP(ssid, password);
    Serial.print("AP SSID: ");
    Serial.println(ssid);
    Serial.print("AP PASSWORD: ");
    Serial.println(password);
    Serial.print("IP address: ");
    Serial.println(WiFi.softAPIP());  //IP address assigned to ATOM
  #else
    WiFiMgrSetup(WiFiAPname);
    Serial.println("connected(^^)");
    Serial.print("AP SSID: ");
    Serial.println(WiFi.SSID());
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());  //IP address assigned to ATOM
  #endif

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
