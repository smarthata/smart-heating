#ifndef SMARTHATA_HEATING_MIXER_H
#define SMARTHATA_HEATING_MIXER_H

#include <DeviceWiFi.h>
#include <Timeout.h>
#include <Interval.h>
#include "MixerRelays.h"
#include "TemperatureSensors.h"


class Mixer : public DeviceWiFi {

private:

    static const int MIXER_CYCLE_TIME = 10000;

    static constexpr float BORDER = 0.1;

    float floorTemp = 25.0;

    MixerRelays mixerRelays = MixerRelays();
    TemperatureSensors sensors = TemperatureSensors();

    Interval interval = Interval(MIXER_CYCLE_TIME);
    Interval readInterval = Interval(1000);

public:

    Mixer(const char *ssid, const char *pass) : DeviceWiFi(ssid, pass) {
        pinMode(LED_BUILTIN, OUTPUT);

        interval.startWithCurrentTimeEnabled();
        readInterval.startWithCurrentTimeEnabled();
    }

    void loop() override {
        DeviceWiFi::loop();

        if (readInterval.isReady()) {
            const SmartHeatingDto &dto = sensors.updateTemperatures();
            checkMixer(dto);
        }

        mixerRelays.loop();
    }

private:

    void checkMixer(const SmartHeatingDto &th) {
        if (sensors.isValidTemp(th.floorMixedTemp) && interval.isReady()) {
            float floorMediumTemp = sensors.calcFloorMediumTemp(th);
            Serial.println(String("Expected floorTemp = ") + floorTemp + "\tfloorMediumTemp = " + floorMediumTemp);
            if (floorMediumTemp < floorTemp - BORDER) {
                float diff = constrain(floorTemp - BORDER - floorMediumTemp, BORDER, 2);
                unsigned int time = calcRelayTime(diff);
                Serial.println(String("UP ") + time + " ms");
                mixerRelays.up(time);
            } else if (floorMediumTemp > floorTemp + BORDER) {
                float diff = constrain(floorMediumTemp - floorTemp - BORDER, BORDER, 2);
                unsigned int time = calcRelayTime(diff);
                Serial.println(String("DOWN ") + time + " ms");
                mixerRelays.down(time);
            } else {
                Serial.println("normal");
            }
        }
    }

    unsigned int calcRelayTime(float diff) const {
        return (unsigned int) mapFloat(diff, BORDER, 2, 1000, 8000);
    }

    float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) const {
        return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
    }

};

#endif
