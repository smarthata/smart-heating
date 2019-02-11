#ifndef SMARTHATA_HEATING_SMARTHATAHEATING_H
#define SMARTHATA_HEATING_SMARTHATAHEATING_H

#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <DeviceWiFi.h>
#include "SmartHataMqtt.h"
#include "config.h"
#include "Mixer.h"
#include "Battery.h"

class SmarthataHeating : public DeviceWiFi {
private:

    Mixer mixer;
    Battery battery;
    TemperatureSensors sensors;
    Interval readInterval = Interval(2000);

    HTTPClient http;
    char buffer[200]{};
    Interval mqttPublishInterval = Interval(30000);
    Interval smarthataPostInterval = Interval(30000);
    Interval narodMonInterval = Interval(300000);

    SmartHataMqtt smartHataMqtt = SmartHataMqtt(mqtt_broker, mqtt_port, mqtt_client_id, mqtt_username, mqtt_password);

public:
    SmarthataHeating(const char *ssid, const char *pass) : DeviceWiFi(ssid, pass, 5000) {
        readInterval.startWithCurrentTime();
        mqttPublishInterval.startWithCurrentTimeEnabled();
        smarthataPostInterval.startWithCurrentTimeEnabled();
        narodMonInterval.startWithCurrentTimeEnabled();
        smartHataMqtt.publish("/messages", "smarthata-heating started", 1);
    }

    void loop() override {
        DeviceWiFi::loop();
        mixer.loop();
        battery.loop();
        smartHataMqtt.loop();

        if (mqttUpdate.floorTempUpdate) {
            String message;
            if (mqttUpdate.floorTemp >= 10 && mqttUpdate.floorTemp <= 35) {
                mixer.floorTemp = mqttUpdate.floorTemp;
                message = String("Setup new floorTemp [") + mqttUpdate.floorTemp + "]";
            } else {
                message = String("Bad request temp [") + mqttUpdate.floorTemp + "]";
            }
            smartHataMqtt.publish("/messages", message, 1);
            mqttUpdate.floorTempUpdate = false;
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
        root["floor-corrected"] = mixer.floorTempCorrected;
        root["mixer-position"] = mixer.getMixerPositionPercentage();
        root["mixer-stabilized"] = mixer.isMixerStabilized();
        addTime(root);
        publish(root);

        jsonBuffer.clear();
        JsonObject &root2 = jsonBuffer.createObject();
        if (TemperatureSensors::isValidTemp(dto.floorMixedTemp)) root2["mixed"] = dto.floorMixedTemp;
        if (TemperatureSensors::isValidTemp(dto.floorColdTemp)) root2["cold"] = dto.floorColdTemp;
        if (TemperatureSensors::isValidTemp(dto.heatingHotTemp)) root2["hot"] = dto.heatingHotTemp;
        if (TemperatureSensors::isValidTemp(dto.streetTemp)) root2["street"] = dto.streetTemp;
        if (TemperatureSensors::isValidTemp(dto.batteryColdTemp)) root2["battery"] = dto.batteryColdTemp;
        if (TemperatureSensors::isValidTemp(dto.boilerTemp)) root2["boiler"] = dto.boilerTemp;
        publish(root2);

        jsonBuffer.clear();
        JsonObject &root3 = jsonBuffer.createObject();
        root3["bedroom-temp"] = mqttUpdate.bedroomTemp;
        root3["bedroom-temp-expected"] = battery.expectedBedroomTemp;
        root3["battery-pomp"] = battery.getBatteryPompState();
        publish(root3);
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

    void postDataToSmarthata(const SmartHeatingDto &dto) {
        sprintf(buffer,
                "http://smarthata.org/api/devices/1/measures?floor=%1.1f&corrected=%1.1f&mixer-position=%d&mixer-stabilized=%d&bedroom-temp-expected=%1.1f&battery-pomp=%d",
                mixer.floorTemp,
                mixer.floorTempCorrected,
                mixer.getMixerPositionPercentage(),
                mixer.isMixerStabilized(),
                battery.expectedBedroomTemp,
                battery.getBatteryPompState()
        );
        makeHttpPostRequest();
        sprintf(buffer,
                "http://smarthata.org/api/devices/1/measures?mixed=%1.1f&cold=%1.1f&battery=%1.1f&heating=%1.1f&boiler=%1.1f&street=%1.1f",
                dto.floorMixedTemp,
                dto.floorColdTemp,
                dto.batteryColdTemp,
                dto.heatingHotTemp,
                dto.boilerTemp,
                dto.streetTemp
        );
        makeHttpPostRequest();
    }

    int postDataToNarodMon(const SmartHeatingDto &dto) {
        Serial.println("Send data to narodmon.ru");
        sprintf(buffer,
                "http://narodmon.ru/get?ID=%s&corrected=%1.1f&mixed=%1.1f&street=%1.1f",
                NARODMON_MAC,
                mixer.floorTempCorrected,
                dto.floorMixedTemp,
                dto.streetTemp
        );
        return makeHttpGetRequest();
    }

    int makeHttpPostRequest(int retryCount = 1) {
        http.begin(buffer);
        int code = http.POST("");
        http.end();

        if (code < 0 && retryCount > 0) {
            return makeHttpPostRequest(retryCount - 1);
        }

        return code;
    }

    int makeHttpGetRequest() {
        http.begin(buffer);
        int code = http.GET();
        http.end();
        return code;
    }

};

#endif
