#define BLYNK_TEMPLATE_ID "TMPL6aTjCwhyM"
#define BLYNK_TEMPLATE_NAME "SmartFarm"
#define BLYNK_AUTH_TOKEN "-pex7OrZpcIFqFe-k3rcmZZxBUiBn-cj"  // Replace with your Blynk Auth Token

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

// WiFi credentials
char ssid[] = "wifi";        // Replace with your WiFi SSID
char pass[] = "wifi.co.id";  // Replace with your WiFi password

// DHT Sensor
#define DHTPIN 15
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// LCD I2C
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Relay pins
#define relay1  13
#define relay2  12
#define relay3  14
#define relay4  27
#define ssrRelay 4

// Button and sensor pins
#define input1  32
#define input2  33
#define input3  25
#define input4  26
#define resetButton  17
#define buzzerPin  2
#define soilSensorPin 34

// Relay states
bool relay1State = LOW;
bool relay2State = LOW;
bool relay3State = LOW;
bool relay4State = LOW;
bool ssrRelayState = LOW;

bool lastButton1State = HIGH;
bool lastButton2State = HIGH;
bool lastButton3State = HIGH;
bool lastButton4State = HIGH;
bool lastResetButtonState = HIGH;

// Timers for relay and SSR control
unsigned long ssrOnDelayStart = 0;
unsigned long relayOffDelayStart = 0;
unsigned long relayOnTime = 0;
unsigned long ssrOnTime = 0;
bool ssrOnDelayActive = false;
bool relayOffDelayActive = false;
bool relayTimerActive = false;
bool ssrTimerActive = false;

// Blynk virtual pins
#define VPIN_HUMIDITY V1
#define VPIN_TEMPERATURE V2
#define VPIN_SOIL_MOISTURE V3
#define VPIN_RELAY1 V4
#define VPIN_RELAY2 V5
#define VPIN_RELAY3 V6
#define VPIN_RELAY4 V7
#define VPIN_SSR V8         // Virtual pin for SSR control
#define VPIN_RESET V9

// Function prototypes
void checkButton(int buttonPin, int relayPin1, int relayPin2, bool &relayState1, bool &relayState2, bool &lastButtonState);
void manageSSR();
void manageRelayTimers();
void buzzForOneSecond();
void resetSystem();

// Blynk write handlers for controlling relays
BLYNK_WRITE(VPIN_RELAY1) {
  relay1State = param.asInt();
  relay3State = param.asInt();
  ssrRelayState = param.asInt();
  digitalWrite(ssrRelay, ssrRelayState);
  digitalWrite(relay1, relay1State);
  digitalWrite(relay3, relay3State);
  buzzForOneSecond();
}

BLYNK_WRITE(VPIN_RELAY2) {
  relay1State = param.asInt();
  relay4State = param.asInt();
  ssrRelayState = param.asInt();
  digitalWrite(ssrRelay, ssrRelayState);
  digitalWrite(relay1, relay1State);
  digitalWrite(relay4, relay4State);
  buzzForOneSecond();
}

BLYNK_WRITE(VPIN_RELAY3) {
  relay2State = param.asInt();
  relay3State = param.asInt();
  ssrRelayState = param.asInt();
  digitalWrite(ssrRelay, ssrRelayState);
  digitalWrite(relay2, relay2State);
  digitalWrite(relay3, relay3State);
  buzzForOneSecond();
}

BLYNK_WRITE(VPIN_RELAY4) {
  relay2State = param.asInt();
  relay4State = param.asInt();
  ssrRelayState = param.asInt();
  digitalWrite(ssrRelay, ssrRelayState);
  digitalWrite(relay2, relay2State);
  digitalWrite(relay4, relay4State);
  buzzForOneSecond();
}


// Blynk reset handler
BLYNK_WRITE(VPIN_RESET) {
  if (param.asInt()) {
    resetSystem();
  }
}

void setup() {
  // Serial for debugging
  Serial.begin(9600);

  // Initialize DHT sensor
  dht.begin();

  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("H: ");
  lcd.setCursor(8, 0);
  lcd.print("T: ");

  // Initialize WiFi and Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Relay pins as output
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  pinMode(relay3, OUTPUT);
  pinMode(relay4, OUTPUT);
  pinMode(ssrRelay, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  // Input pins
  pinMode(input1, INPUT_PULLUP);
  pinMode(input2, INPUT_PULLUP);
  pinMode(input3, INPUT_PULLUP);
  pinMode(input4, INPUT_PULLUP);
  pinMode(resetButton, INPUT_PULLUP);

  resetSystem(); // Initial reset
}

void loop() {
  Blynk.run();

  // DHT sensor readings
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  // Soil moisture reading
  int soilMoistureValue = analogRead(soilSensorPin);
  float soilMoisturePercent = map(soilMoistureValue, 4095, 0, 0, 100);

  // Display on LCD
  if (isnan(humidity) || isnan(temperature)) {
    lcd.setCursor(0, 0);
    lcd.print("Error reading");
  } else {
    lcd.setCursor(2, 0);
    lcd.print(humidity);
    lcd.print("%");
    
    lcd.setCursor(10, 0);
    lcd.print(temperature);
    lcd.print("C");
  }

  lcd.setCursor(0, 1);
  lcd.print("Soil: ");
  lcd.print(soilMoisturePercent);
  lcd.print("%");

  // Update Blynk
  Blynk.virtualWrite(VPIN_HUMIDITY, humidity);
  Blynk.virtualWrite(VPIN_TEMPERATURE, temperature);
  Blynk.virtualWrite(VPIN_SOIL_MOISTURE, soilMoisturePercent);

  // Check button inputs
  checkButton(input1, relay1, relay3, relay1State, relay3State, lastButton1State);
  checkButton(input2, relay1, relay4, relay1State, relay4State, lastButton2State);
  checkButton(input3, relay2, relay3, relay2State, relay3State, lastButton3State);
  checkButton(input4, relay2, relay4, relay2State, relay4State, lastButton4State);

  // Check reset button
  bool currentResetButtonState = digitalRead(resetButton);
  if (currentResetButtonState == LOW && lastResetButtonState == HIGH) {
    delay(50);
    currentResetButtonState = digitalRead(resetButton);
    if (currentResetButtonState == LOW) {
      resetSystem();
    }
  }
  lastResetButtonState = currentResetButtonState;

  manageSSR();
  manageRelayTimers();

  delay(100);
}

void checkButton(int buttonPin, int relayPin1, int relayPin2, bool &relayState1, bool &relayState2, bool &lastButtonState) {
  bool currentButtonState = digitalRead(buttonPin);

  if (currentButtonState == LOW && lastButtonState == HIGH) {
    relayState1 = !relayState1;
    relayState2 = !relayState2;

    if (relayState1 == HIGH && relayState2 == HIGH) {
      digitalWrite(relayPin1, relayState1);
      digitalWrite(relayPin2, relayState2);
      buzzForOneSecond();
      relayOnTime = millis();
      relayTimerActive = true;
      ssrOnDelayActive = true;
      ssrOnDelayStart = millis();
    } else {
      ssrRelayState = LOW;
      digitalWrite(ssrRelay, ssrRelayState);
      buzzForOneSecond();
      relayOffDelayActive = true;
      relayOffDelayStart = millis();
    }
  }

  lastButtonState = currentButtonState;
}

void manageSSR() {
  if (ssrOnDelayActive && (millis() - ssrOnDelayStart >= 3000)) {
    ssrRelayState = HIGH;
    digitalWrite(ssrRelay, ssrRelayState);
    ssrOnTime = millis();
    ssrTimerActive = true;
    ssrOnDelayActive = false;
  }
}

// 15 menit 5 detik
void manageRelayTimers() {
  if (relayTimerActive && (millis() - relayOnTime >= 905000)) {
    relay1State = LOW;
    relay2State = LOW;
    relay3State = LOW;
    relay4State = LOW;

    digitalWrite(relay1, relay1State);
    digitalWrite(relay2, relay2State);
    digitalWrite(relay3, relay3State);
    digitalWrite(relay4, relay4State);

    relayTimerActive = false;
    buzzForOneSecond();
  }
// 15 menit
  if (ssrTimerActive && (millis() - ssrOnTime >= 900000)) {
    ssrRelayState = LOW;
    digitalWrite(ssrRelay, ssrRelayState);
    ssrTimerActive = false;
  }
}

void buzzForOneSecond() {
  digitalWrite(buzzerPin, HIGH);
  delay(1000);
  digitalWrite(buzzerPin, LOW);
}

void resetSystem() {
  relay1State = LOW;
  relay2State = LOW;
  relay3State = LOW;
  relay4State = LOW;
  ssrRelayState = LOW;

  digitalWrite(relay1, relay1State);
  digitalWrite(relay2, relay2State);
  digitalWrite(relay3, relay3State);
  digitalWrite(relay4, relay4State);
  digitalWrite(ssrRelay, ssrRelayState);

  relayTimerActive = false;
  ssrTimerActive = false;
  ssrOnDelayActive = false;
  relayOffDelayActive = false;
}
