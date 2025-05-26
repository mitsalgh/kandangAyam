
#define BLYNK_TEMPLATE_ID "YourTemplateID"
#define BLYNK_DEVICE_NAME "KandangAyam"
#define BLYNK_AUTH_TOKEN "YourAuthToken"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>
#include <Wire.h>
#include <RTClib.h>

// WiFi credentials
char ssid[] = "YourSSID";
char pass[] = "YourPassword";

// Pin configuration
#define DHTPIN 4
#define DHTTYPE DHT22
#define BUZZER_PIN 13
#define RELAY_PIN 12

#define TRIG_PAKAN 14
#define ECHO_PAKAN 27
#define TRIG_AIR 26
#define ECHO_AIR 25

DHT dht(DHTPIN, DHTTYPE);
RTC_DS3231 rtc;

// Virtual Pins Blynk
#define VP_TEMP V0
#define VP_HUMID V1
#define VP_PAKAN V2
#define VP_AIR V3
#define VP_LAMP_SWITCH V4

BlynkTimer timer;

void setupUltrasonic(int trigPin, int echoPin) {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
}

long readDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  return pulseIn(echoPin, HIGH) * 0.034 / 2;
}

void sendSensor() {
  float temp = dht.readTemperature();
  float humid = dht.readHumidity();
  long distPakan = readDistance(TRIG_PAKAN, ECHO_PAKAN);
  long distAir = readDistance(TRIG_AIR, ECHO_AIR);
  DateTime now = rtc.now();

  Blynk.virtualWrite(VP_TEMP, temp);
  Blynk.virtualWrite(VP_HUMID, humid);
  Blynk.virtualWrite(VP_PAKAN, distPakan);
  Blynk.virtualWrite(VP_AIR, distAir);

  // Kondisi jika suhu terlalu panas atau kelembapan terlalu rendah
  if (temp > 35 || humid < 40) {
    digitalWrite(BUZZER_PIN, HIGH);
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }

  // Jika pakan atau air di bawah ambang batas
  if (distPakan > 15 || distAir > 15) {
    digitalWrite(RELAY_PIN, HIGH); // Nyalakan lampu
  } else {
    digitalWrite(RELAY_PIN, LOW);
  }
}

BLYNK_WRITE(VP_LAMP_SWITCH) {
  int lampState = param.asInt();
  digitalWrite(RELAY_PIN, lampState);
}

void setup() {
  Serial.begin(115200);

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);

  setupUltrasonic(TRIG_PAKAN, ECHO_PAKAN);
  setupUltrasonic(TRIG_AIR, ECHO_AIR);

  dht.begin();
  rtc.begin();

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  timer.setInterval(5000L, sendSensor); // Update setiap 5 detik
}

void loop() {
  Blynk.run();
  timer.run();
}
