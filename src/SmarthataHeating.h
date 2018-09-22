#ifndef SMARTHATA_HEATING_SMARTHATAHEATING_H
#define SMARTHATA_HEATING_SMARTHATAHEATING_H

#include <ESP8266HTTPClient.h>
#include <DeviceWiFi.h>
#include "config.h"
#include "Mixer.h"

class SmarthataHeating : public DeviceWiFi {
private:

    Mixer mixer;
    TemperatureSensors sensors;
    Interval readInterval = Interval(2000);

    HTTPClient http;
    char buffer[200]{};
    Interval narodMonInterval = Interval(300000);


public:
    SmarthataHeating(const char *ssid, const char *pass) : DeviceWiFi(ssid, pass, 5000) {
        readInterval.startWithCurrentTime();
        narodMonInterval.startWithCurrentTimeEnabled();
    }

    void loop() override {
        DeviceWiFi::loop();
        mixer.loop();

        if (readInterval.isReady()) {
            const SmartHeatingDto &dto = sensors.updateTemperatures();
            mixer.checkMixer(dto);

            if (narodMonInterval.isReady()) {
                postDataToNarodMon(dto);
            }

        }

    }

private:

    int postDataToNarodMon(const SmartHeatingDto &dto) {
        Serial.println("Send data to narodmon.ru");
        sprintf(buffer,
                "http://narodmon.ru/get?ID=%s&floor=%1.1f&floorMixed=%1.1f&floorCold=%1.1f&batteryCold=%1.1f&street=%1.1f",
                NARODMON_MAC,
                mixer.floorTemp,
                dto.floorMixedTemp,
                dto.floorColdTemp,
                dto.batteryColdTemp,
                dto.streetTemp
        );
        return makeHttpGetRequest();
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
