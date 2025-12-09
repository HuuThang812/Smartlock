#include <WiFi.h>
#include <PubSubClient.h>
int MOSFET_PIN =23;

const char* WIFI_SSID = "";
const char*  WIFI_PASS = "";
const char* mqtt_server = "";
const char* MQTT_USER = "mqtt_user";
const char* MQTT_PASS = "mqtt_pass";

const char* MQTT_TOPIC_SET = "home/door/lock/set";
const char* MQTT_TOPIC_STATE = "home/door/lock/state";
const unsigned long AUTO_RELOCK_MS = 5000;

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long unlockTime = 0;
bool isUnlocked = false;

void callback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];

 Serial.print("MQTT in: ");
  Serial.print(topic);
  Serial.print(" => ");
  Serial.println(msg);

if (String(topic) == MQTT_TOPIC_SET) {
    if (msg == "OPEN" || msg == "ON" || msg == "UNLOCK") {
      digitalWrite(MOSFET_PIN, HIGH);
      isUnlocked = true;
      unlockTime = millis();
      client.publish(MQTT_TOPIC_STATE, "UNLOCKED", true);
      Serial.println("Door: UNLOCKED");
    } 
else if (msg == "CLOSE" || msg == "OFF" || msg == "LOCK") {
      digitalWrite(MOSFET_PIN, LOW);
      isUnlocked = false;
      unlockTime = 0;
      client.publish(MQTT_TOPIC_STATE, "LOCKED", true);
      Serial.println("Door: LOCKED");
    } 
else if (msg.startsWith("OPEN_FOR ")) 
    {
      int ms = msg.substring(9).toInt();
      if (ms > 0 && ms < 120000) {
        digitalWrite(MOSFET_PIN, HIGH);
        isUnlocked = true;
        unlockTime = millis() + ms;
        client.publish(MQTT_TOPIC_STATE, "UNLOCKED", true);
        Serial.printf("Door: UNLOCKED for %d ms\n", ms);
      }
    }
  }
}
void setup() {
  Serial.begin(115200);

  pinMode(MOSFET_PIN, OUTPUT);
  digitalWrite(MOSFET_PIN, LOW); // mặc định khoá
  delay(10);
  setup_wifi()

  client.setServer(MQTT_SERVER, 1883);
  client.setCallback(callback);

}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - start > 20000) {
      Serial.println("\nFailed to connect to WiFi - will retry");
      start = millis();
    }
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32Door-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str(), MQTT_USER, MQTT_PASS)) {
      Serial.println("connected");
      client.subscribe(MQTT_TOPIC_SET);
      client.publish(MQTT_TOPIC_STATE, isUnlocked ? "UNLOCKED" : "LOCKED", true);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println("; retrying in 2s");
      delay(2000);
    }
  }
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

 if (isUnlocked && unlockTime > 0) {
    unsigned long now = millis();
  
    if ((long)(now - unlockTime) >= 0) {
      digitalWrite(MOSFET_PIN, LOW);
      isUnlocked = false;
      unlockTime = 0;
      client.publish(MQTT_TOPIC_STATE, "LOCKED", true);
      Serial.println("Auto re-lock performed");
    }
  }
}
