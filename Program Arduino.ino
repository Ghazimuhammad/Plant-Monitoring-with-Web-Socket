#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"
#include <Arduino_JSON.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_Sensor.h>
#include <WiFi.h>
#include "DHT.h"
#include <time.h>
#include <esp_task_wdt.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

const char* ssid = "POCO M4 Pro";
const char* password = "asdfghjk";
const int moisturePin = 33;

DHT dht(32, DHT11);

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
JSONVar readings;

unsigned long lastTime = 0;
unsigned long timerDelay = 5000;


bool wateringState;
unsigned int moistureThreshold = 50;
int moistureValue;
int moisturePercentage;

//to web socket
void notifyClients(String sensorReadings) {
  ws.textAll(sensorReadings);
}

//handle message from web socket
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo *)arg;if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    String message = (char *)data;

    if (message.startsWith("toggleWatering:")) {
      // Extract the state from the message
      String stateStr = message.substring(strlen("toggleWatering:"));
      wateringState = stateStr.equals("true"); // Convert to boolean
      manualMode(wateringState);
      notifyClients("Watering System State: " + String(wateringState));
    }else if (message.startsWith("setMoistureThreshold:")) {
      String thresholdStr = message.substring(strlen("setMoistureThreshold:"));
      moistureThreshold = thresholdStr.toInt();
      autoMode();
    }
  }
}

//manual mode toggle watering
void manualMode(bool state) {
  if(state){
    digitalWrite(2, LOW);
  }else{
    digitalWrite(2, HIGH);
  }
}

void sendSensorDataToClients(String sensorData) {
  ws.textAll("sensorData:" + sensorData);
}

String getSensorReadings() {
  readings["temperature"] = String(dht.readTemperature());
  readings["humidity"] = String(dht.readHumidity());
  moistureValue = analogRead(moisturePin);
  moisturePercentage = map(moistureValue, 0, 4095, 100, 0);
  readings["moisture"] = String(moisturePercentage);
  String jsonString = JSON.stringify(readings);
  return jsonString;
}

void initSPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  Serial.println("SPIFFS mounted successfully");
}

//handle event from web socket
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

void soilMoisture(){
  int moistureValue_lcd = analogRead(moisturePin);
  int moisturePercentage_lcd = map(moistureValue_lcd, 0, 4095, 100, 0);
  Serial.println(moisturePercentage_lcd);
  lcd.setCursor(0, 0);
  lcd.print("Moisture: ");
  lcd.print(moisturePercentage_lcd);
  lcd.print("%");
}

void autoMode(){
  if (moisturePercentage < moistureThreshold) {
    digitalWrite(2, LOW);  
  } else {
    digitalWrite(2, HIGH);   
  }
  Serial.println(moistureThreshold);
}

void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(1,0);
  lcd.print("System Loading");
  for(int a=0; a <= 15; a++){
    lcd.setCursor(a, 1);
    lcd.print(".");
    delay(200);
  }
  lcd.clear();

  pinMode(2, OUTPUT);

  WiFi.softAP(ssid, password);

  delay(5000);
  initSPIFFS();
  initWebSocket();

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/index.html", "text/html");
  });

  server.on("/sendSensorData", HTTP_POST, [](AsyncWebServerRequest *request){
    String sensorData = request->getParam("sensorData")->value();
    Serial.println("Received Sensor Data: " + sensorData);
    sendSensorDataToClients(sensorData);
    request->send(200, "text/plain", "Data received successfully");
  });

  server.serveStatic("/", SPIFFS, "/");
  server.begin();
  dht.begin();
}

void loop() {
  soilMoisture();
  delay(5000);
  if ((millis() - lastTime) > timerDelay) {
    String sensorReadings = getSensorReadings();
    notifyClients(sensorReadings);
    lastTime = millis();
  }
  ws.cleanupClients();
}
