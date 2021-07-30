#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include <SoftwareSerial.h>
#include <WebSocketsServer.h>

/**
 * REPLACE THIS with a simple header file like the follow
 * 
 * #ifndef _MY_WIFI_KEY_H_
 * #define _MY_WIFI_KEY_H_
 * #define WIFI_SSID "myssid"
 * #define WIFI_KEY "mysecretkey"
 * #endif
 */
#include "/home/devel0/security/mywifikey.h"

/**
 * @brief allow to connect wifi with strongest signal ( add your own in StartWiFi ) 
 */
ESP8266WiFiMulti wifiMulti;

/**
 * @brief basic http web server
 */
ESP8266WebServer server(80);

/**
 * @brief web socket to allow read/write maintaining socket opened 
 */
WebSocketsServer webSocket(81);

/**
 * @brief web socket to allow read analog data A0
 */
WebSocketsServer webSocket2(82);

/**
 * @brief software serial to avoid use the system GPIO1/GPIO3 busy when uploading to esp
 */
SoftwareSerial swSer1;

/**
 * @brief default speed will read from config.txt ( see data folder ) 
 */
uint32_t speed = 9600;

/**
 * @brief default access point ssid,pwd ( this allow to access directly the esp if wifi router not available ) 
 */
const char *softap_ssid = "esp-serial-fwd";
const char *softap_pwd = "123456789";

/**
 * @brief name of device for a mdns search through http://espserial.local 
 */
const char *MDNS_NAME = "espserial";

/**
 * @brief current ws client number ( only 1 client will managed in this simple app ) 
 */
int wsClientId = -1;

/**
 * @brief current ws client number ( only 1 client will managed in this simple app ) 
 */
int wsClientId2 = -1;

/**
 * @brief register wifi ssid,key and enable AP, wait for connection from either wifi or directly to AP then print IP assigned if connected to wifi router
 */
void startWiFi();

/**
 * @brief starts mDNS responder, this allow to reach the device through http://espserial.local 
 */
void startmDNS();

/**
 * @brief activate SPIFFS manager 
 */
void startSPIFFS();

/**
 * @brief read /config.txt ( actually only serial speed saved ) 
 */
void readConfig();

/**
 * @brief closed and reconnect serial with current speed 
 */
void reconnSwSer();

/**
 * @brief start http server process and register handlers for actions get,post 
 */
void startServer();

/**
 * @brief enable ws process 
 */
void startWebSocket();

void setup()
{
  Serial.begin(115200);

  startWiFi();
  startmDNS();
  startSPIFFS();
  readConfig();
  reconnSwSer();
  startServer();
  startWebSocket();
}

#define SERBUF_SIZE 256
uint8_t serbuf[SERBUF_SIZE];
int serbufLen = 0;

uint32_t m_ser_rx = millis();

/**
 * @brief LOOP 
 */
void loop(void)
{
  webSocket.loop();
  webSocket2.loop();
  server.handleClient();
  MDNS.update();

  if (wsClientId != -1)
  {
    if (serbufLen < SERBUF_SIZE - 1 && swSer1.available())
    {
      char c = swSer1.read();

      serbuf[serbufLen++] = c;
    }

    if (serbufLen > 0 && (millis() - m_ser_rx > 500 || serbufLen >= SERBUF_SIZE - 1))
    {
      m_ser_rx = millis();

      Serial.printf("ws sending %d len\n", serbufLen);
      webSocket.sendBIN(wsClientId, serbuf, serbufLen);

      serbufLen = 0;
    }
  }

  if (wsClientId2 != -1)
  {
    auto aval = analogRead(A0);

    auto str = String(aval);

    webSocket2.sendTXT(wsClientId2, str);
  }
}

void startWiFi()
{
  wifiMulti.addAP(WIFI_SSID, WIFI_KEY);
  wifiMulti.addAP(WIFI_SSID2, WIFI_KEY2);

  WiFi.softAP(softap_ssid, softap_pwd);

  Serial.println("Connecting ...");

  while (wifiMulti.run() != WL_CONNECTED && WiFi.softAPgetStationNum() < 1)
  {
    delay(250);
    Serial.print('.');
  }
  Serial.println();

  if (WiFi.softAPgetStationNum() == 0)
  {
    Serial.printf("Connected to [%s] with ip [%s]\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
  }
  else
  {
    Serial.println("Station connected to AP");
  }
}

void startmDNS()
{
  if (MDNS.begin(MDNS_NAME))
    Serial.println("mDNS started");
  else
    Serial.println("mDNS init error");
}

/**
 * @brief states the mime type from file extensions ; returns text/plain if unknown
 */
String getContentType(String filename)
{
  if (filename.endsWith(".htm") || filename.endsWith(".html"))
    return "text/html";
  else if (filename.endsWith(".css"))
    return "text/css";
  else if (filename.endsWith(".js"))
    return "application/javascript";
  else if (filename.endsWith(".ico"))
    return "image/x-icon";

  return "text/plain";
}

/**
 * @brief if path is found then read the content and stream to the server
 * 
 * @param path file on spiffs ( that are deployed through data folder using Project Task platformio/Platform/Upload Filesystem image
 * @return true if found
 * @return false if not found
 */
bool handleFileRead(String path)
{
  Serial.printf("handleFileRead [%s]\n", path.c_str());

  if (path.endsWith("/"))
    path += "index.html";

  auto contentType = getContentType(path);
  if (path.endsWith("config.txt"))
  {
    Serial.println("deny access to config.txt");
  }
  else if (SPIFFS.exists(path))
  {
    auto file = SPIFFS.open(path, "r");
    auto sent = server.streamFile(file, contentType);
    file.close();

    return true;
  }

  Serial.printf("File [%s] not found\n", path.c_str());

  return false;
}

void startSPIFFS()
{
  SPIFFS.begin();
}

void readConfig()
{
  auto file = SPIFFS.open("/config.txt", "r");
  while (file.available())
  {
    auto s = file.readString();
    if (s.startsWith("speed "))
    {
      speed = s.substring(6).toInt();
    }
  }

  file.close();
}

void reconnSwSer()
{
  swSer1.end();
  swSer1.begin(speed, SWSERIAL_8N1, D5, D6, false, 256);
  Serial.printf("swSer1: speed(%d)\n", speed);
}

/**
 * @brief save config to /config.txt 
 */
void saveConfig()
{
  auto file = SPIFFS.open("/config.txt", "w");

  file.printf("speed %d\n", speed);

  file.close();
}

/**
 * @brief handle /serParam? call to save serial speed to config and reconnect serial
 */
void handleSerParam()
{
  auto reconn = false;
  if (server.hasArg("speed"))
  {
    speed = server.arg("speed").toInt();
    Serial.printf("changing speed to [%d]\n", speed);
    reconn = true;
  }

  if (reconn)
  {
    reconnSwSer();
    saveConfig();
  }

  server.send(200);
}

/**
 * @brief handle /config to read config.txt ; data retrieved back to client in json format just for js convenience 
 */
void handleConfig()
{
  String msg;
  msg += "{\"speed\":";
  msg += String(speed);
  msg += "}";
  server.send(200, "application/json", msg);
}

/**
 * @brief handle /netnfo to report ip,mac nfo of wifi card
 */
void handleNetNfo()
{
  String msg;
  msg += "{\"ip\":\"";
  msg += WiFi.localIP().toString();
  msg += "\"";
  msg += ",\"mac\":\"";
  msg += WiFi.macAddress();
  msg += "\"";
  msg += "}";
  server.send(200, "application/json", msg);
}

void handleA0Nfo()
{
  auto aval = analogRead(A0);

  auto str = String(aval);

  server.send(200, "text/plain", str);
}

/**
 * @brief handle /send data POST method that will send to sw serial 
 */
void handleSend()
{
  if (server.hasArg("data"))
  {
    Serial.printf("sending data [%s]\n", server.arg("data").c_str());
    swSer1.println(server.arg("data"));
  }
  server.send(200);
}

void startServer()
{
  server.begin();
  Serial.println("HTTP server started");

  server.on("/serParam", HTTP_GET, handleSerParam);
  server.on("/config", HTTP_GET, handleConfig);
  server.on("/send", HTTP_POST, handleSend);
  server.on("/netnfo", HTTP_GET, handleNetNfo);
  server.on("/a0", HTTP_GET, handleA0Nfo);

  server.onNotFound([]()
                    { 
                      if (!handleFileRead(server.uri()))
                        server.send(404, "text/plain", "404: Not Found");
                    });
}

/**
 * @brief handle websocket events ( actually do nothing apart settings wsClientId on conn/disconn ) 
 */
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t lenght)
{
  switch (type)
  {

  case WStype_DISCONNECTED:
    Serial.printf("[%u] Disconnected!\n", num);
    wsClientId = -1;
    break;

  case WStype_CONNECTED:
  {
    auto ip = webSocket.remoteIP(num);
    wsClientId = num;
    Serial.printf("[%u] connect from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
  }
  break;

  case WStype_TEXT:
    Serial.printf("[%u] get Text: %s\n", num, payload);
    break;
  }
}

/**
 * @brief handle websocket events ( actually do nothing apart settings wsClientId on conn/disconn ) 
 */
void webSocketEvent2(uint8_t num, WStype_t type, uint8_t *payload, size_t lenght)
{
  switch (type)
  {

  case WStype_DISCONNECTED:
    Serial.printf("[%u] (2) Disconnected!\n", num);
    wsClientId2 = -1;
    break;

  case WStype_CONNECTED:
  {
    auto ip = webSocket2.remoteIP(num);
    wsClientId2 = num;
    Serial.printf("[%u] (2) connect from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
  }
  break;
  }
}

void startWebSocket()
{
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  webSocket2.begin();
  webSocket2.onEvent(webSocketEvent2);

  Serial.println("ws started");
}
