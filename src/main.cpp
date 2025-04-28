#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_BME280.h>
#include <BH1750.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// OLED settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Sensor objects
Adafruit_BME280 bme;
BH1750 lightMeter;

// Pin definitions
#define PIR_PIN 27
#define FAN_RELAY_PIN 14
#define LIGHT_RELAY_PIN 12
#define AC_RELAY_PIN 4

// Timing
unsigned long previousMillis = 0;
const long updateInterval = 1000;        
unsigned long lastWeatherAttempt = 0;
const long weatherRetryInterval = 30000; 

// System states
bool fanOn = false;
bool lightOn = false;
bool acOn = false;
float outdoorTemp = -999.0;
float outdoorHum = -999.0;

// WiFi credentials
const char* ssid = "Noname";
const char* password = "1122334455";
const String weatherUrl = "https://api.openweathermap.org/data/2.5/weather?lat=7.2532&lon=80.3454&appid=eca483009d0e5e53599351b8f8f33a30";

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);

  // Initialize OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED fail"));
    while(true);
  }
  display.clearDisplay();
  display.display();

  // Initialize sensors
  if(!bme.begin(0x76) || !lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.println("Sensor init fail!");
    while(1);
  }

  // Initialize I/O
  pinMode(PIR_PIN, INPUT);
  pinMode(FAN_RELAY_PIN, OUTPUT);
  pinMode(LIGHT_RELAY_PIN, OUTPUT);
  pinMode(AC_RELAY_PIN, OUTPUT);

  // Start with relays off
  digitalWrite(FAN_RELAY_PIN, HIGH);
  digitalWrite(LIGHT_RELAY_PIN, HIGH);
  digitalWrite(AC_RELAY_PIN, HIGH);

  Serial.println("System started");
}

void updateWeatherData() {
  if(WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(weatherUrl);
    
    int httpCode = http.GET();
    if(httpCode == 200) {
      String payload = http.getString();
      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, payload);
      
      if(!error) {
        outdoorTemp = doc["main"]["temp"].as<float>() - 273.15;
        outdoorHum = doc["main"]["humidity"].as<float>();
        Serial.println("Weather updated!");
      } else {
        Serial.println("JSON parse error!");
      }
    } else {
      Serial.printf("HTTP error: %d\n", httpCode);
    }
    http.end();
  }
}

void manageWiFi() {
  unsigned long currentMillis = millis();
  
  if(currentMillis - lastWeatherAttempt >= weatherRetryInterval) {
    lastWeatherAttempt = currentMillis;
    
    if(WiFi.status() != WL_CONNECTED) {
      Serial.println("Attempting WiFi connection...");
      WiFi.disconnect();
      WiFi.begin(ssid, password);
    }
    else {
      updateWeatherData();
    }
  }
}

void updateDisplay(float temp, float hum, bool motion) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Indoor info
  display.setCursor(0, 10);
  display.print("Temp:");
  display.print(temp, 1);
  display.print((char)247);
  display.print("C");

  display.setCursor(81, 10);
  display.print("Hum:");
  display.print(hum, 0);
  display.print("%");

  // Horizontal lines
  display.drawLine(0, 20, SCREEN_WIDTH, 20, SSD1306_WHITE);
  display.drawLine(0, 43, SCREEN_WIDTH, 43, SSD1306_WHITE);
  
  // Text sections
  display.setCursor(0, 0);
  display.print("Indoor");
  
  display.setCursor(0, 22);
  display.print("Outdoor");

  // Outdoor info
  display.setCursor(0, 33);
  if(WiFi.status() != WL_CONNECTED) {
    display.print("No WiFi");
  } 
  else if(outdoorTemp == -999.0 || outdoorHum == -999.0) {
    display.print("WiFi Connected !");
  }
  else {
    display.print("Temp:");
    display.print(outdoorTemp, 1);
    display.print((char)247);
    display.print("C");

    display.setCursor(81, 33);
    display.print("Hum:");
    display.print(outdoorHum, 0);
    display.print("%");
  }

  // System status
  display.setCursor(0, 46);
  display.print("Sys: ");
  display.print(motion ? "ON" : "OFF");

  display.setCursor(0, 56);
  display.print("Fan: ");
  display.print(fanOn ? "ON" : "OFF");

  display.setCursor(65, 56);
  display.print("Light: ");
  display.print(lightOn ? "ON" : "OFF");

  display.setCursor(65, 46);
  display.print("AC: ");
  display.print(acOn ? "ON" : "OFF");

  display.display();
}

void loop() {
  unsigned long currentMillis = millis();

  if(currentMillis - previousMillis >= updateInterval) {
    previousMillis = currentMillis;

    // Read sensors
    float temp = bme.readTemperature() - 5.0;
    float hum = bme.readHumidity() + 10.0;
    float lux = lightMeter.readLightLevel();
    bool motion = digitalRead(PIR_PIN);

    // Control relays
    fanOn = temp > 28.0;
    digitalWrite(FAN_RELAY_PIN, fanOn ? LOW : HIGH);
    
    lightOn = motion && (lux < 100);
    digitalWrite(LIGHT_RELAY_PIN, lightOn ? LOW : HIGH);
    
    acOn = temp > 30.0;
    digitalWrite(AC_RELAY_PIN, acOn ? LOW : HIGH);

    // Update display
    updateDisplay(temp, hum, motion);
  }

  // Handle WiFi/weather in background
  manageWiFi();

  // Maintain WiFi connection
  if(WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    client.stop(); 
  }
}
