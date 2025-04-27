#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_BME280.h>
#include <BH1750.h>

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

  Serial.println("System Ready!");
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    float temp = bme.readTemperature()-5.0;      // Temperature in Celsius
    float hum = bme.readHumidity()+10.0;          // Humidity in %
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
    
    // Top section - Temperature, Humidity, and Pressure
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.print("Temp: ");
    display.print(temp, 1); display.print((char)247); display.print("C");

    display.setCursor(0, 10);
    display.print("Humidity: ");
    display.print(hum, 0); display.print("%");

    display.setCursor(0, 20);
    display.print("Pressure: ");
    display.print(pressure, 1); display.print(" hPa");

    // Middle section - Light and Motion
    display.setCursor(0, 30);
    display.print("Light: ");
    display.print(lux, 0); display.print(" lux");

    display.setCursor(0, 40); // Reduced the vertical gap here
    display.print("Motion:");
    display.print(motion ? "Yes" : "No");

    // Bottom section - Device statuses
    display.setCursor(0, 50);
    display.print("Fan: "); display.print(fanOn ? "ON" : "OFF");

    display.setCursor(65, 50);
    display.print("Light: "); display.print(lightOn ? "ON" : "OFF");

    display.setCursor(65, 40); // Adjusted for better placement
    display.print("AC: "); display.print(acOn ? "ON" : "OFF");

    display.display();
  }
}