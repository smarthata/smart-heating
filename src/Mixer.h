#ifndef SMARTHATA_HEATING_MIXER_H
#define SMARTHATA_HEATING_MIXER_H

#include <PIDController.h>
#include <DeviceWiFi.h>
#include <Timeout.h>
#include <Interval.h>
#include "MixerRelays.h"
#include "TemperatureSensors.h"


class MediumValue {
private:
    float value = 0;
    int count = 0;
public:
    void add(double newValue) {
        value += newValue;
        count++;
    }

    double medium() {
        if (count == 0) return 0;
        return value / count;
    }

    void reset() {
        value = 0;
        count = 0;
    }

    double mediumAndReset() {
        double d = medium();
        reset();
        return d;
    }
};

class Mixer : public Arduinable {

private:

    static const uint MIXER_MAX_POSITION_MS = 140000;

    //                        12   1   2   3   4   5  6  7  8  9 10 11
    int8_t corrections[24] = {-3, -3, -2, -2, -2, -1, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    PIDController pid;
    Interval pidInterval = Interval(20UL * 60000);
    Interval mediumValueInterval = Interval(30000);
    MediumValue mediumValue;

    MixerRelays mixerRelays;

    int mixerPosition = 0;

public:

    float floorTemp = 30.0f;
    float floorTempCorrected = floorTemp;
    double valueSec = 0;

    Mixer() {
        pinMode(LED_BUILTIN, OUTPUT);

        mixerRelays.down(MIXER_MAX_POSITION_MS);

        pid.begin();
        pid.tune(25, 0, 0);
        pid.limit(-20, 20);
        pid.setpoint(floorTempCorrected);

        mediumValueInterval.startWithCurrentTimeEnabled();
        pidInterval.startWithCurrentTime();

    }

    void loop() override {
        mixerRelays.loop();
    }

    void checkMixer(const SmartHeatingDto &th) {
        if (TemperatureSensors::isValidTemp(th.floorMixedTemp)) {
            floorTempCorrected = calcFloorTempExpected(th);
            if (mediumValueInterval.isReady()) {
                mediumValue.add(calcFloorMediumTemp(th));
            }
            if (pidInterval.isReady()) {
                pid.setpoint(floorTempCorrected);

                valueSec = pid.compute(mediumValue.mediumAndReset());
                int time = round(valueSec * 1000);
                mixerRelays.run(time);

                mixerPosition = constrain((uint) (mixerPosition + time), 0, MIXER_MAX_POSITION_MS);
            }
        }
    }

    unsigned int getMixerPositionPercentage() {
        return 100 * mixerPosition / MIXER_MAX_POSITION_MS;
    }

private:

    float calcFloorMediumTemp(const SmartHeatingDto &th) const {
        float floorMediumTemp = th.floorMixedTemp;
        if (TemperatureSensors::isValidTemp(th.floorColdTemp))
            floorMediumTemp = (th.floorMixedTemp + th.floorColdTemp) * 0.5f;
        return floorMediumTemp;
    }

    float calcFloorTempExpected(const SmartHeatingDto &th) const {
        float expected = floorTemp;
        if (TemperatureSensors::isValidTemp(th.streetTemp)) {
            expected = expected - th.streetTemp * 0.3f;
        }
        if (mqttUpdate.secondOfDay >= 0) {
            byte hourOfDay = static_cast<byte>(mqttUpdate.secondOfDay / 3600);
            expected = expected + corrections[hourOfDay];
        }
        return expected;
    }


};

#endif
