#ifndef SMARTHATA_HEATING_SMARTHATAHEATING_H
#define SMARTHATA_HEATING_SMARTHATAHEATING_H

#include <DeviceWiFi.h>
#include "Mixer.h"

class SmarthataHeating : public DeviceWiFi {
private:

    Mixer mixer;
    TemperatureSensors sensors;
    Interval readInterval = Interval(2000);

public:
    SmarthataHeating(const char *ssid, const char *pass) : DeviceWiFi(ssid, pass, 5000) {
        readInterval.startWithCurrentTime();
    }

    void loop() override {
        DeviceWiFi::loop();
        mixer.loop();

        if (readInterval.isReady()) {
            const SmartHeatingDto &dto = sensors.updateTemperatures();
            mixer.checkMixer(dto);
        }

    }

};

#endif
