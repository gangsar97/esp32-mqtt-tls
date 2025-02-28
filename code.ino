#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHT.h>

// WiFi Configuration
#define WIFI_SSID "mySSID"
#define WIFI_PASSWORD "myPassword"

// MQTT Configuration
#define MQTT_BROKER "broker.mqtt.com"
#define MQTT_PORT 8883
#define MQTT_USER "username"
#define MQTT_PASS "password"
#define MQTT_TOPIC "topic/subtopic"
#define MQTT_SEND_INTERVAL_MS 60000

// CA Certs
const char* ca_cert = R"EOF(
-----BEGIN CERTIFICATE-----

SIGN YOUR CA CERTIFICATE HERE

-----END CERTIFICATE-----
)EOF";

const char* client_key = R"EOF(
-----BEGIN PRIVATE KEY-----

SIGN YOUR PRIVATE KEY HERE

-----END PRIVATE KEY-----
)EOF";

// DHT Sensor Configuration
#define DHTPIN 14  
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

unsigned long prevMillis = 0;

WiFiClientSecure espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" Connected");


  espClient.setCACert(ca_cert);
  espClient.setCertificate(ca_cert);
  espClient.setPrivateKey(client_key);

  client.setServer(MQTT_BROKER, MQTT_PORT);
  dht.begin();
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    if (client.connect("ESP32Client", MQTT_USER, MQTT_PASS)) {
      Serial.println(" connected");
    } else {
      Serial.print(" failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying in 5 seconds");
      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (millis() - prevMillis >= MQTT_SEND_INTERVAL_MS) {

    float temp = dht.readTemperature();
    float hum = dht.readHumidity();

    if (isnan(temp) || isnan(hum)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }

    StaticJsonDocument<200> jsonDoc;
    char buffer[10];
    sprintf(buffer, "%0.1f", temp);
    jsonDoc["temperature"] = buffer;
    sprintf(buffer, "%0.1f", hum);
    jsonDoc["humidity"] = buffer;

    char jsonBuffer[256];
    serializeJson(jsonDoc, jsonBuffer);

    client.publish(MQTT_TOPIC, jsonBuffer);
    Serial.print("Published: ");
    Serial.println(jsonBuffer);
    prevMillis = millis();
  }

  delay(100);
}
