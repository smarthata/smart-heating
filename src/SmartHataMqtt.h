#ifndef SMARTHATA_MQTT_H
#define SMARTHATA_MQTT_H

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <MQTTClient.h>
#include <Timeout.h>

bool tempSetupByMqtt = false;
float tempFromMqtt = 0;

void messageReceived(String &topic, String &payload) {
    Serial.println("incoming: [" + topic + "] - [" + payload + "]");
    tempFromMqtt = payload.toFloat();
    tempSetupByMqtt = true;
}

class SmartHataMqtt {
private:
    static const int MQTT_CONNECTION_TIMEOUT = 3000;

    WiFiClient net = WiFiClient();
    MQTTClient mqttClient = MQTTClient();

    const char *mqtt_broker;
    const int mqtt_port;
    const char *mqtt_client_id;
    const char *mqtt_username;
    const char *mqtt_password;

public:

    SmartHataMqtt(const char *broker, const int port,
                  const char *client_id, const char *username, const char *password) :
            mqtt_broker(broker), mqtt_port(port),
            mqtt_client_id(client_id), mqtt_username(username), mqtt_password(password) {

        mqttClient.begin(mqtt_broker, mqtt_port, net);
        mqttClient.onMessage(messageReceived);
        loop();
    }

    void loop() {
        mqttClient.loop();
        if (WiFi.isConnected() && !mqttClient.connected()) {
            Serial.println("Connecting MQTT broker [" + String(mqtt_broker) + "]");
            Timeout timeout = Timeout(MQTT_CONNECTION_TIMEOUT);
            while (!mqttClient.connect(mqtt_client_id, mqtt_username, mqtt_password)) {
                Serial.print(".");
                delay(100);
                if (timeout.isReady()) {
                    Serial.println("\n\n\n>>> Connection to MQTT broker [" + String(mqtt_broker) + "] timeout!\n\n\n");
                    return;
                }
            }
            Serial.println();
            Serial.println("MQTT connected!");

            Serial.println("Subscribing MQTT topic");
            if (mqttClient.subscribe("/heating/floor", 1)) {
                Serial.println("MQTT topic subscribed!");
            } else {
                Serial.println("\n\n\n>>> Subscribe to MQTT topic failed!\n\n\n");
            }

        }
    }

    void publish(const char topic[], char message[]) {
        if (mqttClient.connected()) {
            mqttClient.publish(topic, message);
        }
    }

    void publish(const char topic[], const String &message) {
        if (mqttClient.connected()) {
            mqttClient.publish(topic, message);
        }
    }

};


#endif
