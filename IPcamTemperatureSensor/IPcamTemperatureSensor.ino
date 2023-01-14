/*

This is a sketch for ESP8266.

It will read temperature info from DHT22 module and display it in a HikVision IP camera feed.

When powered, it will try to connect to WiFi access point stored in the ESP's memory. When connected, it will start to send
data to a camera, the address of which is also stored in the ESP's memory. If it can't connect to WiFi (you have powered it for the first time or some error occured),
it will create its own access point, so you can configure it on 192.168.1.111 page.

The first temperature update will happen after the update frequency time passes (1 minute default).

Default access point SSID: ESPap-IPcam
Default password: 12345678

DHT22 module is connected to 3,3v , GND and D2 pins.

*/

#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Hash.h>
#include <FS.h>
#include "DHT.h"

#ifndef APSSID
#define APSSID "ESPap-IPcam"
#define APPSK  "12345678"
#endif
#define DHTPIN D2
#define DHTTYPE DHT22

const char *ssid = APSSID;
const char *password = APPSK;

int period;
float temperature = 0;
unsigned long timeNow = 0;

IPAddress local_ip(192,168,1,111);
IPAddress subnet(255,255,255,0);

AsyncWebServer server(80);
DHT dht(DHTPIN, DHTTYPE);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html lang="ru-RU">
<head>
<meta charset="UTF-8" />
		<title>Settings</title>
	<style>
		html {
			max-width: 500px;
			margin: auto;
			font-family: Helvetica;
			background-color: #2274A5;
		}
		
		fieldset {
			width: 400px;
			background-color: #E9F1F7;
			border: 1px solid #131B23;
			border-radius: 10px;
			padding: 20px;
			text-align: right;
			margin-top: 10px;
		}
		
		legend {
			background-color: #E7DFC6;
			border: 1px solid #131B23;
			border-radius: 10px;
			padding: 10px 20px;
			text-align: left;
			text-transform: uppercase;
		}
		input {
			border: 1px solid;
			border-radius: 5px;
			padding-bottom: 5px;
			margin-bottom: 10px;
		}
		.savebutton {
			border: 1px solid;
			border-radius: 10px;
			height: 40px;
			width: 100px;
			color: #000000;
			background-color: #ffffff;
		}
		.savebutton:hover {
			color: #E9F1F7;
			background-color: #131B23;
		}
		.info {
			text-align: left;
			font-size: 12px;
		}
		.fields {
		text-align: right;
		}
	</style>
</head>
<body>
	<form action="/get">
		<fieldset>
		<legend>WiFi Access Point</legend>
		<div class="fields">
			SSID:
			<input type="text" name="input1" size="30" maxlength="100">
			<br />
			Password:
			<input type="password" name="input2" size="30" maxlength="100">
		</div>
			<div class="info">
				Current network: %WIFISSID_TEMPLATE%
        <br />
        Status: %WIFI_STATUS_TEMPLATE%
			</div>
		</fieldset>
		<fieldset>
			<legend>Camera</legend>
			<div class="fields">
				IP
				<input type="text" name="input3" size="30" maxlength="100">
				<br />
				Login
				<input type="text" name="input4" size="30" maxlength="100">
				<br />
				Password
				<input type="password" name="i" size="30" maxlength="100">
			</div>
			<div class="info">
				Current IP: %CAMERAIP_TEMPLATE%
				<br />
				Login: %CAMERALOGIN_TEMPLATE%
				<br />
			  Password: %CAMERAPASSWORD_TEMPLATE%
			</div>
		</fieldset>
		<fieldset>
			<legend>Sensor readings</legend>
			Update frequency (min)
				<input type="freq" name="input6" size="30" maxlength="100">
				<br />
      Air temperature: %TEMPSENSOR_TEMPLATE%°C
	    <div class="info">
				Update frequency (min): %FREQ%
			</div>
		</fieldset>
		<br />
    Warning! You need to fill all of the 6 fields each time, otherwise there might be an error or smth.
    <br />
    You dont need to restart the module if you didnt change the WiFi access point info.
    <br />
		<input type="submit", value="Save" class="savebutton">
		 <br />
		 </form>
		 <form action="/restart">
		 <button class="savebutton">Restart</button>
		 </form>
</body>
</html>)rawliteral";

String processor(const String& var)
{
  if (var == "CAMERAIP_TEMPLATE")
    return readFile(SPIFFS, "/camIP");
  if (var == "CAMERALOGIN_TEMPLATE")
    return readFile(SPIFFS, "/camLogin");
  if (var == "CAMERAPASSWORD_TEMPLATE")
    return "***";
  if (var == "WIFISSID_TEMPLATE")
    return readFile(SPIFFS, "/wifiSSID");
  if (var == "TEMPSENSOR_TEMPLATE")
    return String(temperature);
  if (var == "FREQ")
    return readFile(SPIFFS, "/updFreq");
  if (var == "WIFI_STATUS_TEMPLATE")
    if (WiFi.status() == WL_CONNECTED)
      return "Connected.";
    else return "Failed to connect.";
  return String();
}

String readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\r\n", path);
  File file = fs.open(path, "r");
  if(!file || file.isDirectory()){
    Serial.println("- empty file or failed to open file");
    return String("-99999");
  }
  String fileContent;
  while(file.available()){
    fileContent+=String((char)file.read());
  }
  return fileContent;
}

void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\r\n", path);
  File file = fs.open(path, "w");
  if(!file){
    Serial.println("- failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
}

int getUpdPeriod(){
  int updFreq = readFile(SPIFFS, "/updFreq").toInt();
  if (updFreq == -99999){
    writeFile(SPIFFS, "/updFreq", "10");
    return 600000;
  }
  if (updFreq > 1440){
    writeFile(SPIFFS, "/updFreq", "1440");
    return 1440 * 60000;
  }
  if (updFreq < 1){
    writeFile(SPIFFS, "/updFreq", "1");
    return 60000;
  }
  return updFreq * 60000;
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT); 
  pinMode(DHTPIN, INPUT);

  if(!SPIFFS.begin()){
      Serial.println("An Error has occurred while mounting SPIFFS");
      return;
  }

  dht.begin();

  WiFi.mode(WIFI_STA);
  WiFi.begin(readFile(SPIFFS, "/wifiSSID"), readFile(SPIFFS, "/wifiPass"));
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("WiFi Failed!");
    Serial.println("Creating softAP");
     WiFi.softAPConfig(local_ip, local_ip, subnet);
     WiFi.softAP(ssid, password);
  }
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());




  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    int params = request->params();
    for (int i = 0; i < params; i++){
      AsyncWebParameter* p = request->getParam(i);
      String message = request->getParam(i)->value();
      int messageLength = message.length() + 1;
      switch (i){
        case 0:
        writeFile(SPIFFS, "/wifiSSID", message.c_str());
        break;
        case 1:
        writeFile(SPIFFS, "/wifiPass", message.c_str());
        break;
        case 2:
        writeFile(SPIFFS, "/camIP", message.c_str());
        break;
        case 3:
        writeFile(SPIFFS, "/camLogin", message.c_str());
        break;
        case 4:
        writeFile(SPIFFS, "/camPass", message.c_str());
        break;
        case 5:
        writeFile(SPIFFS, "/updFreq", message.c_str());
        getUpdPeriod();
        break;
      }
    }
     request->send_P(200, "text/html", index_html, processor);
  });

  server.on("/restart", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("Restarting now...");
    ESP.restart();
  });

   server.begin();
   period = getUpdPeriod();
}

void loop() {
  if (WiFi.status() == WL_CONNECTED){
    if (millis() - timeNow > period){
      timeNow = millis();
      temperature = dht.readTemperature();
      Serial.println(dht.readTemperature());

      WiFiClient client;

      HTTPClient http;

      Serial.print("[HTTP] begin...\n");

      String url = "http://" + readFile(SPIFFS, "/camLogin") + ":" + readFile(SPIFFS, "/camPass") + "@" + readFile(SPIFFS, "/camIP") + "/Video/inputs/channels/1/overlays/text";
      if (http.begin(client, url)) {
        Serial.print("[HTTP] GET...\n");

        int httpCode = http.GET();

       // httpCode will be negative on error
        if (httpCode > 0) {
        Serial.printf("[HTTP] GET... code: %d\n", httpCode);

        if (httpCode == 200) {

          String str = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
          str += "<TextOverlay version=\"1.0\" xmlns=\"http://www.hikvision.com/ver10/XMLSchema\">\n";
          str += "<id>1</id>\n";
          str += "<enabled>true</enabled>\n";
          str += "<posX>16</posX>\n";
          str += "<posY>544</posY>\n";
          str += "<message>Temp: " + String(temperature) + "°C</message>\n";
          str += "</TextOverlay>";
          int httpCode2 = http.PUT(str); 
         Serial.printf("[HTTP] PUT... code: %d\n", httpCode2);
        }

         if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
           String payload = http.getString();
            Serial.println(payload);
         }
        } else {
         Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      }

      http.end();
    } else {
      Serial.printf("[HTTP} Unable to connect\n");
    }
  }
  }
}
