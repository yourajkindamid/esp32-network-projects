#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "SCONET";
const char* password = "111122ff";

const int output16 = 16;
const int output17 = 17;
String output16State = "off";
String output17State = "off";
String outputtogglestate = "off";

WebServer server(80);

void handleGPIO16On() {
  output16State = "on";
  digitalWrite(output16, HIGH);
  handleRoot();
}

void handleGPIO16Off() {
  output16State = "off";
  digitalWrite(output16, LOW);
  handleRoot();
}

void handleGPIO17On() {
  output17State = "on";
  digitalWrite(output17, HIGH);
  handleRoot();
}

void handleGPIO17Off() {
  output17State = "off";
  digitalWrite(output17, LOW);
  handleRoot();
}

void patternhandler() 
{
    outputtogglestate = "on";
    digitalWrite(output17, LOW);
    digitalWrite(output16, LOW);
    delay(2000);
    digitalWrite(output16, HIGH);
    delay(2000);
    digitalWrite(output17, HIGH);
    delay(2000);
    digitalWrite(output16, LOW);
    digitalWrite(output17, LOW);
    handleRoot();

}

void patternkill()
{
  outputtogglestate = "off";
  handleRoot();
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  html += "<link rel=\"icon\" href=\"data:,\">";
  html += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}";
  html += ".button { background-color: #4b5fd2; border: none; color: white; padding: 16px 40px; text-decoration: none; font-size: 100px; margin: 2px; cursor: pointer;}";
  html += ".button2 { background-color: #c28080; }</style></head>";
  html += "<body><h1>Web Server Based on ESP32</h1>";

  html += "<p>GPIO 16 - State " + output16State + "</p>";
  if (output16State == "off") {
    html += "<p><a href=\"/16/on\"><button class=\"button\">OFF</button></a></p>";
  } else {
    html += "<p><a href=\"/16/off\"><button class=\"button button2\">ON</button></a></p>";
  }

  html += "<p>GPIO 17 - State " + output17State + "</p>";
  if (output17State == "off") {
    html += "<p><a href=\"/17/on\"><button class=\"button\">OFF</button></a></p>";
  } else {
    html += "<p><a href=\"/17/off\"><button class=\"button button2\">ON</button></a></p>";
  }

  html += "<p>TOGGLE PATTERN - State " + outputtogglestate + "</p>";
  if (outputtogglestate == "off") {
    html += "<p><a href=\"/toggle/on\"><button class=\"button\">START</button></a></p>";
  } else {
    html += "<p><a href = \"/toggle/off\"><button class=\"button button2\">STOPPED</button></a></p>";
  }

  html += "</body></html>";
  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);

  pinMode(output16, OUTPUT);
  pinMode(output17, OUTPUT);
  digitalWrite(output16, LOW);
  digitalWrite(output17, LOW);

  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/16/on", handleGPIO16On);
  server.on("/16/off", handleGPIO16Off);
  server.on("/17/on", handleGPIO17On);
  server.on("/17/off", handleGPIO17Off);
  server.on("/toggle/on", patternhandler);
  server.on("/toggle/off", patternkill);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}