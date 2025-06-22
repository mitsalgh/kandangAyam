#define BLYNK_TEMPLATE_ID "TMPL6LDCfxaFV"
#define BLYNK_TEMPLATE_NAME "Kandang Ayam"
#define BLYNK_AUTH_TOKEN "cix4z8n7umkWXcnCHb_P6B6og6HznQh3"

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <DHT22.h>
#include <Wire.h>
#include <RTClib.h>




// WiFi credentials
char ssid[] = "WISMA WIJAYA";
char pass[] = "12345678";

// Pin configuration
#define DHTPIN 4
#define DHTTYPE DHT22
#define BUZZER_PIN 13
#define RELAY_PIN 12

#define TRIG_PAKAN 14
#define ECHO_PAKAN 27
#define TRIG_AIR 26
#define ECHO_AIR 25

DHT22 dht(DHTPIN);
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
  float temp = dht.getTemperature();
  float humid = dht.getHumidity();
  long distPakan = readDistance(TRIG_PAKAN, ECHO_PAKAN);
  long distAir = readDistance(TRIG_AIR, ECHO_AIR);
  DateTime now = rtc.now();

  // Konversi ke presentase (wadah: 20cm = kosong, 2cm = penuh)
  int persentasePakan = constrain(map(distPakan, 20, 2, 0, 100), 0, 100);
  int persentaseAir = constrain(map(distAir, 20, 2, 0, 100), 0, 100);

  Serial.print(persentasePakan);
  Serial.print(persentaseAir);

  // Kirim ke Blynk
  Blynk.virtualWrite(VP_TEMP, temp);
  Blynk.virtualWrite(VP_HUMID, humid);
  Blynk.virtualWrite(VP_PAKAN, persentasePakan);
  Blynk.virtualWrite(VP_AIR, persentaseAir);

  // Buzzer aktif jika suhu >35 atau kelembapan <40
  if (temp > 35 || humid < 40) {
    digitalWrite(BUZZER_PIN, HIGH);
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }

  // Lampu aktif jika pakan atau air kurang dari 30%
  if (persentasePakan < 30 || persentaseAir < 30) {
    digitalWrite(RELAY_PIN, HIGH);
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

  // dht.begin();
  rtc.begin();

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  timer.setInterval(5000L, sendSensor); // Update setiap 5 detik
}

void loop() {
  Blynk.run();
  timer.run();
}
