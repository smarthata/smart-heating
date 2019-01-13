#ifndef SMARTHATA_HEATING_SMARTHATAHEATING_H
#define SMARTHATA_HEATING_SMARTHATAHEATING_H

#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <DeviceWiFi.h>
#include "SmartHataMqtt.h"
#include "config.h"
#include "Mixer.h"

class SmarthataHeating : public DeviceWiFi {
private:

    Mixer mixer;
    TemperatureSensors sensors;
    Interval readInterval = Interval(2000);

    HTTPClient http;
    char buffer[200]{};
    Interval mqttPublishInterval = Interval(20000);
    Interval smarthataPostInterval = Interval(30000);
    Interval narodMonInterval = Interval(300000);

    SmartHataMqtt smartHataMqtt = SmartHataMqtt(mqtt_broker, mqtt_port, mqtt_client_id, mqtt_username, mqtt_password);

public:
    SmarthataHeating(const char *ssid, const char *pass) : DeviceWiFi(ssid, pass, 5000) {
        readInterval.startWithCurrentTime();
        mqttPublishInterval.startWithCurrentTimeEnabled();
        smarthataPostInterval.startWithCurrentTimeEnabled();
        narodMonInterval.startWithCurrentTimeEnabled();
    }

    void loop() override {
        DeviceWiFi::loop();
        mixer.loop();
        smartHataMqtt.loop();

        if (tempSetupByMqtt) {
            String message;
            if (tempFromMqtt >= 10 && tempFromMqtt <= 35) {
                mixer.floorTemp = tempFromMqtt;
                message = String("Setup new floorTemp [") + tempFromMqtt + "]";
            } else {
                message = String("Bad request temp [") + tempFromMqtt + "]";
            }
            Serial.println(message);
            smartHataMqtt.publish("/messages", message);
            tempSetupByMqtt = false;
        }

        if (readInterval.isReady()) {
            const SmartHeatingDto &dto = sensors.updateTemperatures();
            mixer.checkMixer(dto);

            if (mqttPublishInterval.isReady()) {
                publish(dto);
            }

            if (TemperatureSensors::isValidTemp(dto.streetTemp)) {
                if (narodMonInterval.isReady()) {
                    postDataToNarodMon(dto);
                }
            }

            if (smarthataPostInterval.isReady()) {
                postDataToSmarthata(dto);
            }
        }

    }

private:

    void publish(const SmartHeatingDto &dto) {
        DynamicJsonBuffer jsonBuffer;

        JsonObject &root = jsonBuffer.createObject();
        root["deviceId"] = device_id;
        root["temp"] = mixer.floorTemp;
        root["corrected"] = mixer.floorTempCorrected;
        addTime(root);
        publish(root);

        jsonBuffer.clear();
        JsonObject &root2 = jsonBuffer.createObject();
        root2["deviceId"] = device_id;
        if (TemperatureSensors::isValidTemp(dto.floorMixedTemp)) root2["mixed"] = dto.floorMixedTemp;
        if (TemperatureSensors::isValidTemp(dto.floorColdTemp)) root2["cold"] = dto.floorColdTemp;
        if (TemperatureSensors::isValidTemp(dto.heatingHotTemp)) root2["hot"] = dto.heatingHotTemp;
        if (TemperatureSensors::isValidTemp(dto.streetTemp)) root2["street"] = dto.streetTemp;
        if (TemperatureSensors::isValidTemp(dto.batteryColdTemp)) root2["battery"] = dto.batteryColdTemp;
        if (TemperatureSensors::isValidTemp(dto.boilerTemp)) root2["boiler"] = dto.boilerTemp;
        publish(root2);
    }

    void addTime(JsonObject &root) {
        unsigned long sec = millis() / 1000;
        unsigned long mins = sec / 60;
        unsigned long hours = mins / 60;
        unsigned long days = hours / 24;
        sec %= 60;
        mins %= 60;
        hours %= 24;
        if (days > 0) {
            root["days"] = days;
        } else if (hours > 0) {
            root["hours"] = hours;
        } else if (mins > 0) {
            root["mins"] = mins;
        } else {
            root["sec"] = sec;
        }
    }

    void publish(const JsonObject &root) {
        String message;
        root.printTo(message);
        Serial.println(message);
        smartHataMqtt.publish("/heating/floor", message);
    }

    int postDataToSmarthata(const SmartHeatingDto &dto) {
        sprintf(buffer,
                "http://smarthata.org/api/devices/1/measures?floor=%1.1f&corrected=%1.1f&mixed=%1.1f&cold=%1.1f&battery=%1.1f&heating=%1.1f&boiler=%1.1f&street=%1.1f",
                mixer.floorTemp,
                mixer.floorTempCorrected,
                dto.floorMixedTemp,
                dto.floorColdTemp,
                dto.batteryColdTemp,
                dto.heatingHotTemp,
                dto.boilerTemp,
                dto.streetTemp
        );
        return makeHttpPostRequest();
    }

    int postDataToNarodMon(const SmartHeatingDto &dto) {
        Serial.println("Send data to narodmon.ru");
        sprintf(buffer,
                "http://narodmon.ru/get?ID=%s&floor=%1.1f&corrected=%1.1f&mixed=%1.1f&street=%1.1f",
                NARODMON_MAC,
                mixer.floorTemp,
                mixer.floorTempCorrected,
                dto.floorMixedTemp,
                dto.streetTemp
        );
        return makeHttpGetRequest();
    }

    int makeHttpPostRequest() {
        Serial.println(buffer);
        http.begin(buffer);
        int code = http.POST("");
        Serial.println(code);
        http.end();
        return code;
    }

    int makeHttpGetRequest() {
        Serial.println(buffer);
        http.begin(buffer);
        int code = http.GET();
        Serial.println(code);
        http.end();
        return code;
    }

};

#endif
