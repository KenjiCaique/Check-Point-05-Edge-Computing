/*
 * FIAP - Edge Computing & Computer Systems - Checkpoint 05
 * Vinheria Agnello - IoT (ESP32 + DHT11 + LDR + MQTT)
 *
 * Bibliotecas (Gerenciador de Bibliotecas da Arduino IDE):
 *  - PubSubClient (Nick O'Leary)
 *  - DHT sensor library (Adafruit) + Adafruit Unified Sensor
 */
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <time.h>

// ===================== CONFIGURAÇÃO (ALTERE AQUI) =====================
const char* WIFI_SSID = "FIAP-IOT";
const char* WIFI_PASS = "F!@p25.IOT";

const char* MQTT_HOST = "3.135.234.101";  // IP do broker Mosquitto
const uint16_t MQTT_PORT = 1883;
const char* MQTT_USER = "";   // deixe vazio se não houver autenticação
const char* MQTT_PASS = "";

const char* DEVICE_ID = "adega001";
const char* API_KEY   = "TEF";   // apikey do serviço no IoT Agent

// Tópicos (padrão IoT Agent MQTT - JSON)
String TOPIC_ATTRS  = String("/json/") + API_KEY + "/" + DEVICE_ID + "/attrs";
String TOPIC_CMD    = String("/") + API_KEY + "/" + DEVICE_ID + "/cmd";
String TOPIC_CMDEXE = String("/") + API_KEY + "/" + DEVICE_ID + "/cmdexe";
// ======================================================================

// Pinos (conforme montagem do slide 10)
#define DHT_PIN  4
#define LDR_PIN  34
#define LED_PIN  2
#define DHT_TYPE DHT11

const unsigned long PUBLISH_INTERVAL_MS = 5000;

DHT dht(DHT_PIN, DHT_TYPE);
WiFiClient espClient;
PubSubClient mqtt(espClient);
unsigned long lastPublish = 0;

void connectWiFi() {
  Serial.printf("Conectando ao Wi-Fi %s ", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.printf("\nWi-Fi OK. IP: %s\n", WiFi.localIP().toString().c_str());
}

void syncTime() {
  // UTC; o NGSI usa ISO8601 em UTC (TimeInstant)
  configTime(0, 0, "pool.ntp.org", "time.google.com");
  Serial.print("Sincronizando horario (NTP)");
  time_t now = time(nullptr);
  while (now < 1700000000) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println(" OK");
}

String isoTimestamp() {
  time_t now = time(nullptr);
  struct tm t;
  gmtime_r(&now, &t);
  char buf[25];
  strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &t);
  return String(buf);
}

void onMessage(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
  Serial.printf("[MQTT] %s -> %s\n", topic, msg.c_str());

  // Aceita comandos como "adega001@on|" / "adega001@off|" ou JSON contendo on/off
  String lower = msg;
  lower.toLowerCase();
  String cmd = "";
  if (lower.indexOf("off") >= 0) {
    digitalWrite(LED_PIN, LOW);
    cmd = "off";
  } else if (lower.indexOf("on") >= 0) {
    digitalWrite(LED_PIN, HIGH);
    cmd = "on";
  }
  if (cmd != "") {
    String resp = String(DEVICE_ID) + "@" + cmd + "|" + cmd;
    mqtt.publish(TOPIC_CMDEXE.c_str(), resp.c_str());
  }
}

void connectMQTT() {
  while (!mqtt.connected()) {
    Serial.print("Conectando ao broker MQTT...");
    String clientId = String("esp32-") + DEVICE_ID + "-" + String((uint32_t)ESP.getEfuseMac(), HEX);
    bool ok = (strlen(MQTT_USER) > 0)
                ? mqtt.connect(clientId.c_str(), MQTT_USER, MQTT_PASS)
                : mqtt.connect(clientId.c_str());
    if (ok) {
      Serial.println(" OK");
      mqtt.subscribe(TOPIC_CMD.c_str());
      Serial.printf("Inscrito em %s\n", TOPIC_CMD.c_str());
    } else {
      Serial.printf(" falhou (rc=%d). Nova tentativa em 3s\n", mqtt.state());
      delay(3000);
    }
  }
}

void publishReadings() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  int raw = analogRead(LDR_PIN);              // 0..4095
  int luminosity = map(raw, 0, 4095, 0, 100); // % (maior = mais claro; inverta se sua montagem for oposta)

  if (isnan(t) || isnan(h)) {
    Serial.println("Falha na leitura do DHT11. Leitura ignorada.");
    return;
  }

  String ts = isoTimestamp();
  String payload = "{\"temperature\":" + String(t, 1) +
                   ",\"humidity\":" + String(h, 1) +
                   ",\"luminosity\":" + String(luminosity) +
                   ",\"TimeInstant\":\"" + ts + "\"}";

  bool ok = mqtt.publish(TOPIC_ATTRS.c_str(), payload.c_str());
  Serial.printf("[PUB] %s %s\n", ok ? "OK" : "ERRO", payload.c_str());
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(LDR_PIN, INPUT);
  dht.begin();

  connectWiFi();
  syncTime();

  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(onMessage);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  if (!mqtt.connected()) connectMQTT();
  mqtt.loop();

  unsigned long now = millis();
  if (now - lastPublish >= PUBLISH_INTERVAL_MS) {
    lastPublish = now;
    publishReadings();
  }
}
