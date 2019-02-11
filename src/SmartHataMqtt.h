#ifndef SMARTHATA_MQTT_H
#define SMARTHATA_MQTT_H

#include <ESP8266WiFi.h>
#include <ESP8266httpUpdate.h>
#include <WiFiClient.h>
#include <MQTTClient.h>
#include <Timeout.h>


struct MqttUpdate {
    int secondOfDay = -1;

    bool firmwareUpdate = false;

    bool floorTempUpdate = false;
    float floorTemp = 0;

    float bedroomTemp = -127.0f;
    float bedroomHum = -127.0f;
} mqttUpdate;


void messageReceived(String &topic, String &payload) {
    Serial.print("incoming: [" + topic + "] - [" + payload + "]\n");
    if (topic.equals("/heating/floor/in")) {
        if (payload.equals("update")) {
            mqttUpdate.firmwareUpdate = true;
        } else if (payload.equals("restart")) {
            ESP.restart();
        } else {
            mqttUpdate.floorTemp = payload.toFloat();
            mqttUpdate.floorTempUpdate = true;
        }
    } else if (topic.equals("/second-of-day")) {
        mqttUpdate.secondOfDay = payload.toInt();

    } else if (topic.equals("/room/bedroom")) {
        DynamicJsonBuffer jsonBuffer;
        const JsonVariant &root = jsonBuffer.parse(payload);
        mqttUpdate.bedroomHum = root["hum"];
        mqttUpdate.bedroomTemp = root["temp"];
    }

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
        if (mqttUpdate.firmwareUpdate) {
            doUpdate();
        }

        if (WiFi.isConnected() && !mqttClient.connected()) {
            Timeout timeout = Timeout(MQTT_CONNECTION_TIMEOUT);
            while (!mqttClient.connect(mqtt_client_id, mqtt_username, mqtt_password)) {
                DEBUG_SH(".");
                delay(100);
                if (timeout.isReady()) {
                    return;
                }
            }

            Serial.print(F("Subscribing MQTT topic\n"));
            subs("/heating/floor/in", 1);
            subs("/room/bedroom");
            subs("/second-of-day");
        }
    }

    void subs(const char *topic, int qos = 0) {
        if (mqttClient.subscribe(topic, qos)) {
            Serial.print(F("MQTT topic subscribed!\n"));
        } else {
            Serial.print(F("\n\n\n>>> Subscribe to MQTT topic failed!\n\n\n"));
        }
    }

    void publish(const char topic[], const char message[], int qos = 0) {
        if (mqttClient.connected()) {
            mqttClient.publish(topic, message, false, qos);
        }
    }

    void publish(const char topic[], const String &message, int qos = 0) {
        this->publish(topic, message.c_str(), qos);
    }

    void doUpdate() {
        this->publish("/heating/floor/message", "Update smarthata-heating from smarthata.org", 1);

        t_httpUpdate_return ret = ESPhttpUpdate.update(firmware);
        switch (ret) {
            case HTTP_UPDATE_FAILED:
                Serial.print(F("[update] Update failed.\n"));
                this->publish("/heating/floor/message", "[update] Update failed.", 1);
                break;
            case HTTP_UPDATE_NO_UPDATES:
                Serial.print(F("[update] Update no Update.\n"));
                this->publish("/heating/floor/message", "[update] Update no Update.", 1);
                break;
            case HTTP_UPDATE_OK:
                this->publish("/heating/floor/message", "[update] Update ok.", 1);
                Serial.print(F("[update] Update ok.\n")); // may not called we reboot the ESP
                break;
        }

        mqttUpdate.firmwareUpdate = false;
    }

};


#endif
