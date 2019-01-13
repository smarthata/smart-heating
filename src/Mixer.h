#ifndef SMARTHATA_HEATING_MIXER_H
#define SMARTHATA_HEATING_MIXER_H

#include <DeviceWiFi.h>
#include <Timeout.h>
#include <Interval.h>
#include "MixerRelays.h"
#include "TemperatureSensors.h"


class Mixer : public Arduinable {

private:

    static const int MIXER_CYCLE_SAFE_DELAY_MS = 2000;

    static constexpr float BORDER = 0.2f;

    MixerRelays mixerRelays;
    Timeout cycleTimeout;

public:

    float floorTemp = 30.0f;
    float floorTempCorrected = floorTemp;

    Mixer() {
        pinMode(LED_BUILTIN, OUTPUT);

        mixerRelays.disable();

    }

    void loop() override {
        mixerRelays.loop();
    }

    void checkMixer(const SmartHeatingDto &th) {
        if (cycleTimeout.isReady() && TemperatureSensors::isValidTemp(th.floorMixedTemp)) {
            float floorMediumTemp = calcFloorMediumTemp(th);

            calcFloorTempCorrected(th);

            Serial.println(
                    String("Expected floorTemp = ") + floorTempCorrected +
                    "\tfloorMediumTemp = " + floorMediumTemp +
                    "\tfloorTempCorrected = " + floorTempCorrected
            );
            if (floorMediumTemp < floorTempCorrected - BORDER) {
                float diff = constrain(floorTempCorrected - BORDER - floorMediumTemp, BORDER, 2);
                unsigned int time = calcRelayTime(diff);
                Serial.println(String("UP ") + time + " ms");
                mixerRelays.up(time);
                cycleTimeout.start(time + MIXER_CYCLE_SAFE_DELAY_MS);
            } else if (floorMediumTemp > floorTempCorrected + BORDER) {
                float diff = constrain(floorMediumTemp - floorTempCorrected - BORDER, BORDER, 2);
                unsigned int time = calcRelayTime(diff);
                Serial.println(String("DOWN ") + time + " ms");
                mixerRelays.down(time);
                cycleTimeout.start(time + MIXER_CYCLE_SAFE_DELAY_MS);
            } else {
                Serial.println("normal");
            }
        }
    }

private:

    unsigned int calcRelayTime(float diff) const {
        return (unsigned int) mapFloat(diff, BORDER, 2, 1000, 5000);
    }

    float calcFloorMediumTemp(const SmartHeatingDto &th) const {
        float floorMediumTemp = th.floorMixedTemp;
        if (TemperatureSensors::isValidTemp(th.floorColdTemp))
            floorMediumTemp = (th.floorMixedTemp + th.floorColdTemp) * 0.5f;
        return floorMediumTemp;
    }

    float calcFloorTempCorrected(const SmartHeatingDto &th) {
        floorTempCorrected = floorTemp;
        if (TemperatureSensors::isValidTemp(th.streetTemp)) {
            floorTempCorrected = floorTemp - th.streetTemp * 0.3f;
        }
        return floorTempCorrected;
    }

    float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) const {
        return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
    }

};

#endif
