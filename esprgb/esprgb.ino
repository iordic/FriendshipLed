#include <EEPROM.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define START_ADDRESS 0
#define MAGIC_NUMBER 1312

// Output pins
uint8_t redLed = D6;
uint8_t blueLed = D7;
uint8_t greenLed = D8;

struct configuration {
  int magic;
  boolean set;
  char ssid[32];
  char password[32];
  char mqttServer[48];
  int mqttPort;
  char topic[64];
} Config;

WiFiClient espClient;
PubSubClient client(espClient);

StaticJsonDocument<512> doc;


void(* resetFunc) (void) = 0; //declare reset function @ address 0 (good ñapa)

void setup_wifi(char *ssid, char *password) {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect(char *clientId, char *inTopic) {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(clientId)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe(inTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  uint32_t num = (uint32_t) strtol((char *) payload, NULL, 16);  // Convert hex string to int
  num &= 0x00ffffff;
  Serial.print("Setting color to: ");
  Serial.println(num);
  integerToRGB(num);
}

void setup() {
  Serial.begin(115200);
  EEPROM.begin(512);  // ESP8266 has 512 bytes
  EEPROM.get(START_ADDRESS, Config);  // Load struct from EEPROM
  if (Config.magic == MAGIC_NUMBER) {
    Serial.println("\n* Configuration loaded.");
    Serial.print("Magic number: ");
    Serial.println(Config.magic);
    Serial.print("¿Configured?: ");
    Serial.println(Config.set);
    Serial.print("Wifi SSID: ");
    Serial.println(Config.ssid);
    Serial.print("Wifi password: ");
    Serial.println(Config.password);
    Serial.print("MQTT Server address: ");
    Serial.println(Config.mqttServer);
    Serial.print("MQTT Port: ");
    Serial.println(Config.mqttPort);
    Serial.print("Subscribed topic: ");
    Serial.println(Config.topic);
    // TO-DO: Connect to wifi
    setup_wifi(Config.ssid, Config.password);
    client.setServer(Config.mqttServer, Config.mqttPort);
    client.setCallback(callback);
  } else {
    Serial.println("\nError on load, magic number mismatch, use serial to configure.");
    Config.set = 0;
  }
  EEPROM.end();
}

void loop() {
  if (Config.set && !client.connected()) {
    reconnect("testclient1", Config.topic);
  }
  client.loop();

  if (Serial.available()) {
    String raw = Serial.readStringUntil('\n');
    DeserializationError error = deserializeJson(doc, raw);
    if (error) {
      Serial.print("Error on DeserializationError: ");
      Serial.println(error.f_str());
    } else {
      int code = doc["code"];
      switch (code) {
        // Set configuration
        case 1:
          if (doc.containsKey("magic") && doc.containsKey("ssid") && doc.containsKey("password") &&
              doc.containsKey("mqtt_server") && doc.containsKey("mqtt_port") && doc.containsKey("topic")) {
            // Fill the configuration structure
            JsonObject root = doc.as<JsonObject>();
            // Copy values to structure
            Config.magic = root["magic"];
            copyJsonValueToCharArray("ssid", Config.ssid, root);
            copyJsonValueToCharArray("password", Config.password, root);
            copyJsonValueToCharArray("mqtt_server", Config.mqttServer, root);
            Config.mqttPort = root["mqtt_port"];
            copyJsonValueToCharArray("topic", Config.topic, root);
            Config.set = true;
            // Save changes to EEPROM
            EEPROM.begin(512);  // ESP8266 has 512 bytes
            EEPROM.put(START_ADDRESS, Config);
            EEPROM.commit();
            EEPROM.end();
            Serial.println("Resetting device...");
            resetFunc();  // call reset
          } else {
            Serial.println("Error: missing fields.");
          }
          break;
        case 2: // Set color manualy, format in hex: RRGGBB (hex value of byte for each color)
          if (doc.containsKey("color")) {
            const char *p = doc["color"];
            uint32_t num = (uint32_t) strtol(p, NULL, 16);  // Convert hex string to int
            num &= 0x00ffffff;
            Serial.print("Setting color to: ");
            Serial.println(num);
            integerToRGB(num);
          } else {
            Serial.println("Error: missing color key.");
          }
          break;
      }
    }
  }
}

/* Copy json value to char array dest (given its key) */
void copyJsonValueToCharArray(String key, char *dest, JsonObject src) {
  String s = src[key].as<String>();
  int n = s.length() + 1;
  s.toCharArray(dest, n);
}

/* Convert uint raw value to RGB, values are: 0x00RRGGBB */
void integerToRGB(uint32_t value) {
  uint8_t rColor, gColor, bColor;
  // Apply masks and shift to obtain each value
  rColor = (value >> 16) & 0xff;
  gColor = (value >> 8) & 0xff;
  bColor = value & 0xff;
  // Write values to PWM output after map (PWM has values 0-1023)
  analogWrite(redLed, map(rColor, 0, 255, 0, 1023));
  analogWrite(greenLed, map(gColor, 0, 255, 0, 1023));
  analogWrite(blueLed, map(bColor, 0, 255, 0, 1023));
}
