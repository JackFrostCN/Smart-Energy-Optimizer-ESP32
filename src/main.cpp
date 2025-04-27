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

// BME280 sensor
Adafruit_BME280 bme;

// BH1750 light sensor
BH1750 lightMeter;

// PIR motion sensor
#define PIR_PIN 27

// Relay pins
#define FAN_RELAY_PIN 14
#define LIGHT_RELAY_PIN 12
#define AC_RELAY_PIN 4

// Timer
unsigned long previousMillis = 0;
const long interval = 1000; // 1s update

// Relay states
bool fanOn = false;
bool lightOn = false;
bool acOn = false;

// Outdoor weather details
float outdoorTemp = 0.0;
float outdoorHum = 0.0;

// WiFi credentials
const char* ssid = "Noname";
const char* password = "1122334455";

// OpenWeather API URL
String weatherUrl = "https://api.openweathermap.org/data/2.5/weather?lat=7.2532&lon=80.3454&appid=eca483009d0e5e53599351b8f8f33a30";

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("OLED fail"));
    while(true);
  }
  display.clearDisplay();
  display.display();

  if (!bme.begin(0x76)) {
    Serial.println("BME280 fail");
    while(1);
  }

  if (!lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.println("BH1750 fail");
    while(1);
  }

  pinMode(PIR_PIN, INPUT);

  pinMode(FAN_RELAY_PIN, OUTPUT);
  pinMode(LIGHT_RELAY_PIN, OUTPUT);
  pinMode(AC_RELAY_PIN, OUTPUT);

  digitalWrite(FAN_RELAY_PIN, HIGH);
  digitalWrite(LIGHT_RELAY_PIN, HIGH);
  digitalWrite(AC_RELAY_PIN, HIGH);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi Connected");

  Serial.println("System Ready!");
}

void getOutdoorWeather() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(weatherUrl); // OpenWeatherMap API URL

    int httpCode = http.GET();  // Send GET request
    if (httpCode == 200) {
      String payload = http.getString();
      // Use DynamicJsonDocument for dynamic allocation
      DynamicJsonDocument doc(1024);

      DeserializationError error = deserializeJson(doc, payload);

      if (!error) {
        // Convert temperature from Kelvin to Celsius
        outdoorTemp = doc["main"]["temp"].as<float>() - 273.15;
        outdoorHum = doc["main"]["humidity"].as<float>();
      } else {
        Serial.println("Error parsing JSON");
      }
    } else {
      Serial.println("Failed to get weather data");
    }
    http.end();
  } else {
    Serial.println("WiFi Disconnected");
  }
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    float temp = bme.readTemperature() - 5.0;      // Temperature in Celsius
    float hum = bme.readHumidity() + 10.0;          // Humidity in %
    float pressure = bme.readPressure() / 100.0F;  // Pressure in hPa
    float lux = lightMeter.readLightLevel();  // Light in lux
    bool motion = digitalRead(PIR_PIN);      // Motion detected or not

    // ---- Relay Logic ----
    if (temp > 28.0) {
      digitalWrite(FAN_RELAY_PIN, LOW); fanOn = true;
    } else {
      digitalWrite(FAN_RELAY_PIN, HIGH); fanOn = false;
    }

    // Light control logic
    if (motion && lux < 100) {
      digitalWrite(LIGHT_RELAY_PIN, LOW); // Light on
      lightOn = true;
    } else {
      digitalWrite(LIGHT_RELAY_PIN, HIGH); // Light off
      lightOn = false;
    }

    if (temp > 30.0) {
      digitalWrite(AC_RELAY_PIN, LOW); acOn = true;
    } else {
      digitalWrite(AC_RELAY_PIN, HIGH); acOn = false;
    }

    // ---- OLED Display ----
    display.clearDisplay();

    if (WiFi.status() == WL_CONNECTED) {
      getOutdoorWeather(); // Get outdoor weather only when WiFi is connected
    }

    // Display Indoor Sensor Data
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    // Indoor section - Temperature, Humidity, and Pressure
    display.setCursor(0, 0);
    display.print("Temp:");
    display.print(temp, 1); display.print((char)247); display.print("C");

    display.setCursor(80, 0);
    display.print("Hum:");
    display.print(hum, 0); display.print("%");

    display.setCursor(0, 10);
    display.print("Air Pres:");
    display.print(pressure, 1); display.print(" hPa");

    display.drawLine(0, 30, SCREEN_WIDTH, 30, SSD1306_WHITE);
    display.drawLine(0, 43, SCREEN_WIDTH, 43, SSD1306_WHITE);
    
    // Light Level
     display.setCursor(0, 20);
     display.print("Light: ");
     display.print(lux, 0); display.print(" lux");
    
    // Outdoor section - Temperature and Humidity
    display.setCursor(0, 33);
    display.print("O Temp:");
    display.print(outdoorTemp, 1); display.print((char)247); display.print("C");

    display.setCursor(80, 33);
    display.print("Hum:");
    display.print(outdoorHum, 0); display.print("%");

   

    // System Status (S for system on/off)
    display.setCursor(0, 46);
    display.print("Sys: ");
    display.print(motion ? "ON" : "OFF");

    // Device statuses
    display.setCursor(0, 56);
    display.print("Fan: "); display.print(fanOn ? "ON" : "OFF");

    display.setCursor(65, 56);
    display.print("Light: "); display.print(lightOn ? "ON" : "OFF");

    display.setCursor(65, 46);
    display.print("AC: "); display.print(acOn ? "ON" : "OFF");

    // Show "No WiFi" if WiFi is disconnected
    if (WiFi.status() != WL_CONNECTED) {
      display.setCursor(0, 60);
      display.setTextSize(1);
      display.print("No WiFi");
    }

    display.display();
  }
}
