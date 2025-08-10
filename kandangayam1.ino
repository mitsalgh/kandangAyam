#define BLYNK_TEMPLATE_ID "TMPL6FVNQ6qlo"
#define BLYNK_TEMPLATE_NAME "Kandang Ayam"
#define BLYNK_AUTH_TOKEN "wfOx1SXocTpO95GLvSlEdzgcAt63ddzZ"

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <DHT22.h>
#include <Wire.h>
#include <RTClib.h>
#include <HTTPClient.h>

// WiFi credentials
char ssid[] = "OPPO Reno4 F";
char pass[] = "wawan123";

int lampState;
bool flagSwitch = 0; // 0 = normal, 1 = override manual
int flagStateOn=0;



// Pin configuration
#define DHTPIN 4
#define DHTTYPE DHT22
#define BUZZER_PIN 2
#define RELAY_PIN 33

#define TRIG_PAKAN 5
#define ECHO_PAKAN 18
#define TRIG_AIR 19
#define ECHO_AIR 23

DHT22 dht(DHTPIN);
RTC_DS3231 rtc;

// Virtual Pins Blynk
#define VP_TEMP V0
#define VP_HUMID V1
#define VP_PAKAN V2
#define VP_AIR V3
#define VP_LAMP_SWITCH V4

/*This is the Sheets GAS ID, you need to look for your sheets id*/                       
String Sheets_GAS_ID = "AKfycby8WJqjWXybLkXVEEu5QsCOIZKU6pJ4EcfKyVnieDFt2NZ_WYKbQGoyVpWSZbyKAnRo";   
float suhu = 0.0, kelembaban = 0.0, sisaPakan = 0.0, sisaMinum = 0.0;
long int cntTimer5min = 0;
long int timer5sec = 0;


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

// void readSensor() {
//   float temp = dht.getTemperature();
//   float humid = dht.getHumidity();
//   long distPakan = readDistance(TRIG_PAKAN, ECHO_PAKAN);
//   long distAir = readDistance(TRIG_AIR, ECHO_AIR);

//   int persentasePakan = constrain(map(distPakan, 20, 6, 0, 100), 0, 100);
//   int persentaseAir = constrain(map(distAir, 20, 2, 0, 100), 0, 100);

//   Serial.print("Humidity : ");
//   Serial.print(humid);
//   Serial.print("  | Temp : ");
//   Serial.print(temp);
//   Serial.print("  | Pakan: ");
//   Serial.print(persentasePakan);
//   Serial.print("% | Air: ");
//   Serial.print(persentaseAir);
//   Serial.println("%");
// }

void sendSensor() {
  float temp = dht.getTemperature();
  float humid = dht.getHumidity();
  long distPakan = readDistance(TRIG_PAKAN, ECHO_PAKAN);
  long distAir = readDistance(TRIG_AIR, ECHO_AIR);


  Serial.print("pakan : ");
  Serial.print(distPakan);
  Serial.print("  Minum : ");
  Serial.print(distAir);
  Serial.println();
  int persentasePakan = constrain(map(distPakan, 13, 6, 0, 100), 0, 100); //ganti setting pakan
  int persentaseAir = constrain(map(distAir, 13, 6, 0, 100), 0, 100); //ganti setting minum

   suhu = (float) temp;
  kelembaban = (float) humid;
  sisaPakan = (float) persentasePakan;
  sisaMinum = (float) persentaseAir;
  Serial.println("---------------------------------");
  Serial.print("Data ke Spreadsheet : ");
  Serial.print(suhu);
  Serial.print(" \t");
  Serial.print(kelembaban);
  Serial.print(" \t");
  Serial.print(sisaPakan);
  Serial.print(" \t");
  Serial.println(sisaMinum);
  Serial.println("---------------------------------");

  Serial.print("Pakan: ");
  Serial.print(persentasePakan);
  Serial.print("% | Air: ");
  Serial.print(persentaseAir);
  Serial.print("% | Temp: ");
  Serial.print(temp);
  Serial.print("°C | Humid: ");
  Serial.print(humid);
  Serial.println("%");

  Blynk.virtualWrite(VP_TEMP, temp);
  Blynk.virtualWrite(VP_HUMID, humid);
  Blynk.virtualWrite(VP_PAKAN, persentasePakan);
  Blynk.virtualWrite(VP_AIR, persentaseAir);

  // Logika lampu otomatis hanya jika tidak override
  if (flagSwitch == 0) {
    if(temp < 33 && flagStateOn==0 )
    {
      digitalWrite(RELAY_PIN, HIGH);
    }
    else if (temp >= 33 || humid < 40) { //set point temp
      digitalWrite(RELAY_PIN, HIGH);  // matikan lampu
      flagStateOn=1;
    } else if (temp == 30 && flageStateOn==1)  {
      flagStateOn=0;   // nyalakan lampu
    }
  }

  // Buzzer aktif jika pakan atau air < 30%
  if (persentasePakan < 30 || persentaseAir < 30) {
    digitalWrite(BUZZER_PIN, HIGH);
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }
}

// Manual control via Blynk (override 10 detik)
BLYNK_WRITE(VP_LAMP_SWITCH) {
  lampState = param.asInt();
  Serial.println("Manual switch state: " + String(lampState));

  flagSwitch = 1; // Masuk ke mode override

  if (lampState == 1) {
    digitalWrite(RELAY_PIN, LOW);  // Paksa lampu ON
  } else {
    digitalWrite(RELAY_PIN, HIGH);   // Paksa lampu OFF
  }

  timer.setTimeout(10000L, resetSwitch); // Kembali ke mode normal setelah 10 detik
}

void resetSwitch() {
  flagSwitch = 0;
  Serial.println("Kembali ke mode otomatis (suhu dan kelembaban)");
}

void connectToWiFi() {
  Serial.print("Menghubungkan ke WiFi");
  WiFi.begin(ssid, pass);
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 20) {
    delay(500);
    Serial.print(".");
    retry++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi terhubung!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    digitalWrite(BUZZER_PIN, LOW);
    delay(200);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(200);
  } else {
    Serial.println("\nGagal koneksi WiFi.");
  }
}

void checkConnection() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi terputus! Mencoba koneksi ulang...");
    connectToWiFi();
  }

  if (!Blynk.connected()) {
    Serial.println("Blynk tidak terhubung! Mencoba koneksi ulang...");
    Blynk.connect();
  }

  if (WiFi.status() == WL_CONNECTED && Blynk.connected()) {
    Serial.println("Koneksi ke WiFi dan Blynk OK.");
  }
}

void WRITE_GoogleSheet(){
  String urlFinal = "https://script.google.com/macros/s/"+Sheets_GAS_ID+"/exec?"+"value1=" + String(suhu) + "&value2=" + String(kelembaban) + "&value3=" + String(sisaPakan) + "&value4=" + String(sisaMinum);
  Serial.print("POST data to spreadsheet:");
  Serial.println(urlFinal);
  HTTPClient http;
  http.begin(urlFinal.c_str());
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  int httpCode = http.GET(); 
  Serial.print("HTTP Status Code: ");
  Serial.println(httpCode);

  String payload;
  if (httpCode > 0) {
      payload = http.getString();
      Serial.println("Payload: "+payload);    
  }
  http.end();
}

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(35, OUTPUT);
  digitalWrite(35, HIGH);
  pinMode(RELAY_PIN, OUTPUT);

  setupUltrasonic(TRIG_PAKAN, ECHO_PAKAN);
  setupUltrasonic(TRIG_AIR, ECHO_AIR);

  connectToWiFi();
  Blynk.config(BLYNK_AUTH_TOKEN);
  Blynk.connect();

  timer.setInterval(1000L, sendSensor);
  timer.setInterval(10000L, checkConnection);
}

void loop() {
  Blynk.run();
  timer.run();

  if((millis() - timer5sec) > 5000){
    cntTimer5min++;
    if(cntTimer5min >= 60){
      // Upload data ke spreadsheet
      WRITE_GoogleSheet();
      cntTimer5min = 0;
      Serial.println("Send Gsheet");
    }
    timer5sec = millis();
    
  }
  
}
