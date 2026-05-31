#include <WiFi.h>
#include <HTTPClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ===================== PINS =====================

#define TRIG_PIN 5
#define ECHO_PIN 18

#define RAIN_PIN 34
#define MQ2_PIN 35
#define MQ3_PIN 32

#define MODE_BUTTON_PIN 22

#define COOLING_PUMP_PIN 19
#define BUZZER_PIN 21
#define LED_PIN 4

#define ONE_WIRE_BUS 23

// I2C OLED
#define SDA_PIN 25
#define SCL_PIN 26

// ===================== OLED =====================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ===================== WIFI =====================

const char* ssid = "test";
const char* password = "12345678";
const char* serverURL = "http://YOUR_PC_IP:5000/api/data";

// ===================== WINE MODE =====================

bool whiteWineMode = true;
bool lastButtonState = HIGH;

const float WHITE_WINE_MAX_TEMP = 18.0;
const float RED_WINE_MAX_TEMP = 28.0;

// ===================== DS18B20 =====================

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds18b20(&oneWire);

// ===================== FUNCTIONS =====================

float getDistanceCM() {

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);

  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);

  if (duration == 0) return -1;

  return duration * 0.0343 / 2.0;
}

float getTemperature() {
  ds18b20.requestTemperatures();
  return ds18b20.getTempCByIndex(0);
}

// ===================== DISPLAY =====================

void updateDisplay(float temp) {

  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);

  if (whiteWineMode) {
    display.println("WHITE");
  } else {
    display.println("RED");
  }

  display.setTextSize(1);
  display.println("Wine Mode");

  display.print("Temp: ");
  display.print(temp);
  display.println(" C");

  display.print("Limit: ");

  if (whiteWineMode)
    display.println("18 C");
  else
    display.println("28 C");

  display.display();
}

// ===================== BUTTON =====================

void checkModeButton() {

  bool current = digitalRead(MODE_BUTTON_PIN);

  if (lastButtonState == HIGH && current == LOW) {

    whiteWineMode = !whiteWineMode;

    Serial.print("Mode: ");
    Serial.println(whiteWineMode ? "WHITE WINE" : "RED WINE");

    delay(250); // debounce
  }

  lastButtonState = current;
}

// ===================== CONTROL =====================

void controlTemperature(float temp) {

  float maxTemp = whiteWineMode ? WHITE_WINE_MAX_TEMP : RED_WINE_MAX_TEMP;

  if (temp > maxTemp) {

    digitalWrite(COOLING_PUMP_PIN, HIGH);
    digitalWrite(LED_PIN, HIGH);
    tone(BUZZER_PIN, 500);

  } else {

    digitalWrite(COOLING_PUMP_PIN, LOW);
    digitalWrite(LED_PIN, LOW);
    noTone(BUZZER_PIN);
  }
}

// ===================== WIFI =====================

void connectWiFi() {

  Serial.print("Connecting WiFi");

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected!");
}

// ===================== SEND DATA =====================

void sendData(float temperature, float distance, int mq2, int mq3, int rain) {

  if (WiFi.status() != WL_CONNECTED) return;

  String json = "{";

  json += "\"temperature\":" + String(temperature, 1) + ",";
  json += "\"distance\":" + String(distance, 1) + ",";
  json += "\"mq2\":" + String(mq2) + ",";
  json += "\"mq3\":" + String(mq3) + ",";
  json += "\"rain\":" + String(rain) + ",";
  json += "\"status\":\"ONLINE\"";

  json += "}";

  HTTPClient http;
  http.begin(serverURL);
  http.addHeader("Content-Type", "application/json");

  int code = http.POST(json);

  Serial.println("HTTP: " + String(code));

  http.end();
}

// ===================== SETUP =====================

void setup() {

  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(MODE_BUTTON_PIN, INPUT_PULLUP);

  pinMode(COOLING_PUMP_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(COOLING_PUMP_PIN, LOW);
  digitalWrite(LED_PIN, LOW);

  Wire.begin(SDA_PIN, SCL_PIN);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED FAIL");
    while (true);
  }

  ds18b20.begin();
  connectWiFi();
}

// ===================== LOOP =====================

unsigned long lastSend = 0;

void loop() {

  checkModeButton();

  float temperature = getTemperature();
  float distance = getDistanceCM();

  int mq2 = analogRead(MQ2_PIN);
  int mq3 = analogRead(MQ3_PIN);
  int rain = analogRead(RAIN_PIN);

  controlTemperature(temperature);
  updateDisplay(temperature);

  if (millis() - lastSend > 5000) {

    sendData(temperature, distance, mq2, mq3, rain);
    lastSend = millis();
  }
}