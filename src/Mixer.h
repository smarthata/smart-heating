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
    static const uint MIXER_MAX_POSITION_MS = 140000;

    static constexpr float BORDER = 0.5f;
    //                        12   1   2   3   4   5  6  7  8  9 10 11
    int8_t corrections[24] = {-3, -3, -2, -2, -2, -1, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    MixerRelays mixerRelays;
    Timeout cycleTimeout;

    int mixerPosition = 0;
    bool mixerStabilized = false;

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

            if (floorMediumTemp < floorTempCorrected - BORDER && limitTopMixerPosition(80)) {
                float diff = constrain(floorTempCorrected - BORDER - floorMediumTemp, BORDER, 2);
                unsigned int time = calcRelayTime(diff);
                mixerRelays.up(time);
                cycleTimeout.start(time + MIXER_CYCLE_SAFE_DELAY_MS);
                mixerPosition = constrain((uint) (mixerPosition + time), 0, MIXER_MAX_POSITION_MS);
                checkStabilization();
            } else if (floorMediumTemp > floorTempCorrected + BORDER && limitDownMixerPosition(20)) {
                float diff = constrain(floorMediumTemp - floorTempCorrected - BORDER, BORDER, 2);
                unsigned int time = calcRelayTime(diff);
                mixerRelays.down(time);
                cycleTimeout.start(time + MIXER_CYCLE_SAFE_DELAY_MS);
                mixerPosition = constrain((uint) (mixerPosition - time), 0, MIXER_MAX_POSITION_MS);
            }
        }
    }

    unsigned int getMixerPositionPercentage() {
        return 100 * mixerPosition / MIXER_MAX_POSITION_MS;
    }

    int isMixerStabilized() const {
        return mixerStabilized ? 10 : 0;
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

    void calcFloorTempCorrected(const SmartHeatingDto &th) {
        floorTempCorrected = floorTemp;
        if (TemperatureSensors::isValidTemp(th.streetTemp)) {
            floorTempCorrected = floorTemp - th.streetTemp * 0.3f;
        }
        if (mqttUpdate.secondOfDay >= 0) {
            byte hourOfDay = static_cast<byte>(mqttUpdate.secondOfDay / 3600);
            floorTempCorrected = floorTempCorrected + corrections[hourOfDay];
        }
    }

    bool limitTopMixerPosition(int percentage) const {
        return !mixerStabilized || mixerPosition < percentage * MIXER_MAX_POSITION_MS / 100;
    }

    bool limitDownMixerPosition(int percentage) const {
        return !mixerStabilized || mixerPosition > percentage * MIXER_MAX_POSITION_MS / 100;
    }

    void checkStabilization() {
        if (mixerPosition == MIXER_MAX_POSITION_MS) {
            mixerStabilized = true;
        }
    }

    float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) const {
        return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
    }

};

#endif
