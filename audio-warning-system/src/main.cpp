#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "secret.h"
#include <cstring>
#include <ArduinoJson.h>

// Vars begin
// Network clients
WiFiClient wifi_client;
PubSubClient mqtt_client(wifi_client);

// Neopixel
constexpr uint8_t LED_PIN = 27;
constexpr uint8_t NUMPIX = 1;

Adafruit_NeoPixel strip(NUMPIX, LED_PIN, NEO_GRB + NEO_KHZ800);

// Vars end

// Functions:
void mqtt_connect();
void wifi_connect();
void neopixel_connect();
void neopixel_write(uint32_t col, uint8_t brightness);
void callback(char* topic, byte* payload, unsigned int length);

void setup()
{
  // Setup:
  Serial.begin(115200);
  while (!Serial)
  {
    delay(1);
  }

  Serial.println("Serial connected successfully\n");
  neopixel_connect();
  neopixel_write(strip.Color(255, 0, 0), 128);
  wifi_connect();
  neopixel_write(strip.Color(255, 255, 0), 128);
  mqtt_connect();

  uint32_t color = strip.Color(1, 71, 185);
  neopixel_write(color, 128);
}

void loop()
{
  // Loop:
  // Ensure connection
  if (WiFi.status() != WL_CONNECTED){
    Serial.println("WiFi connection dropped, reconnecting...");
    wifi_connect();
  }

  if (!mqtt_client.connected()) {
    Serial.println("MQTT broker connection dropped, reconnecting...");
    mqtt_connect();
  }

  mqtt_client.loop();
}

// Initialise neopixel
void neopixel_connect()
{
  strip.begin();
  strip.show();
}

// Connect to wifi
void wifi_connect(){
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("WiFi connected");
}

// Connect to MQTT broker
void mqtt_connect()
{
  // Set MQTT server
  mqtt_client.setServer(MQTT_BROKER_ADDRESS, MQTT_BROKER_PORT);
  Serial.println("MQTT server sat");
  // If connected successfully, proceed
  if (mqtt_client.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD)){
    Serial.println("MQTT broker connected");
  } else {
    // If not connected, retry until connection is made
    while (true) {
      Serial.print("Failed to connect to MQTT broker, rc=");
      Serial.println(mqtt_client.state());
      delay(500); // Delay to not overload server
      if (mqtt_client.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
        Serial.println("MQTT broker connected");
        break;
      }
    }
  }
  JsonDocument doc;
  doc["payload"] = "connected";
  doc["client"] = MQTT_CLIENT_ID;
  char msg[128];
  serializeJson(doc, msg);
  mqtt_client.publish("esp/status", msg);
  mqtt_client.subscribe("esp/commands");
  mqtt_client.setCallback(callback);
}

void neopixel_write(uint32_t col, uint8_t brightness){
  strip.setPixelColor(0, col);
  strip.setBrightness(brightness);
  strip.show();
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message at [");
  Serial.print(topic);
  Serial.println("]");

  char msg[length + 1];
  memcpy(msg, payload, length);
  msg[length] = '\0';

  JsonDocument doc;
  deserializeJson(doc, msg);

  const char* command = doc["command"];
  if (strcmp(command, "set_colour")) {
    uint32_t col = strip.Color(doc["red"], doc["green"], doc["blue"]);
    uint8_t bright = doc["brightness"];
    neopixel_write(col, bright);
    Serial.print("Color set to: ");
    Serial.println(col);
  }
}