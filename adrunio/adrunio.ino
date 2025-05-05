
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Servo.h>
#include "HX711.h"
#include "DHT.h"

// === Khai báo chân ===
#define TRIG_PIN_1 D1
#define ECHO_PIN_1 D2
#define TRIG_PIN_2 D0
#define ECHO_PIN_2 D8
#define SERVO_PIN D4
#define BUZZER_PIN D3
#define DT_PIN D5
#define SCK_PIN D6
#define DHTPIN D7
#define DHTTYPE DHT11

// === Khởi tạo thiết bị ===
Servo myServo;
HX711 scale;
DHT dht(DHTPIN, DHTTYPE);

WiFiClient espClient;
PubSubClient client(espClient);

bool daPhatNhac = false;

// === Thông tin WiFi và MQTT ===
const char* ssid = "VIETTEL_5PT";
const char* password = "4JRF6EGW";
const char* mqtt_server = "192.168.1.6"; //  Không có khoảng trắng
const int mqtt_port = 1883;

// === Hàm phát nhạc ===
void phatNhacUiIaAa() {
  int melody[] = { 330, 660, 880, 523, 392, 440 };
  int noteDurations[] = { 300, 300, 300, 300, 300, 500 };

  for (int i = 0; i < 6; i++) {
    tone(BUZZER_PIN, melody[i], noteDurations[i]);
    delay(noteDurations[i] * 1.3);
    noTone(BUZZER_PIN);
  }
}

// === Đo khoảng cách cảm biến siêu âm ===
float getDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 30000);
  return duration * 0.034 / 2;
}

// === Kết nối WiFi và MQTT ===
void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Dang ket noi MQTT...");
    if (client.connect("ESP8266Client")) {
      Serial.println("thanh cong!");
    } else {
      Serial.print("That bai, thu lai sau 5s. Loi: ");
      Serial.println(client.state());
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(9600);

  pinMode(TRIG_PIN_1, OUTPUT);
  pinMode(ECHO_PIN_1, INPUT);
  pinMode(TRIG_PIN_2, OUTPUT);
  pinMode(ECHO_PIN_2, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  noTone(BUZZER_PIN);

  myServo.attach(SERVO_PIN);
  myServo.write(0);

  scale.begin(DT_PIN, SCK_PIN);
  scale.set_scale(1000.0);
  delay(500);
  scale.tare();
  delay(500);

  dht.begin();

  // Kết nối WiFi
  WiFi.begin(ssid, password);
  Serial.print("Dang ket noi WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Da ket noi WiFi!");

  // Kết nối MQTT
  client.setServer(mqtt_server, mqtt_port);

  Serial.println("Khoi dong hoan tat!");
}

void loop() {
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();

  float distance1 = getDistance(TRIG_PIN_1, ECHO_PIN_1);
  float distance2 = getDistance(TRIG_PIN_2, ECHO_PIN_2);
  float weight = scale.get_units();
  float doAm = dht.readHumidity();
  float nhietDo = dht.readTemperature();

  if (distance1 > 0 && distance1 < 10) {
    myServo.write(180);
    if (!daPhatNhac) {
      phatNhacUiIaAa();
      daPhatNhac = true;
    }
  } else {
    myServo.write(0);
    daPhatNhac = false;
  }

  if (doAm > 83.0) {
    tone(BUZZER_PIN, 1000);
  } else if (distance1 <= 0 || distance1 >= 10) {
    noTone(BUZZER_PIN);
  }

  // Gửi dữ liệu qua MQTT
  client.publish("sensor/distance1", String(distance1).c_str(), true);
  client.publish("sensor/distance2", String(distance2).c_str(), true);
  client.publish("sensor/weight", String(weight).c_str(), true);
  client.publish("sensor/temperature", String(nhietDo).c_str(), true);
  client.publish("sensor/humidity", String(doAm).c_str(), true);

  Serial.println("----");
  delay(1000);
}
