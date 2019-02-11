#ifndef SMARTHATA_HEATING_BATTERY_H
#define SMARTHATA_HEATING_BATTERY_H

class Battery {

public:

    static constexpr float NORMAL_TEMP = 21.0f;
    static constexpr float BORDER = 0.2f;

    float expectedBedroomTemp = NORMAL_TEMP;
    //                        12 1  2  3  4  5  6  7  8  9 10 11
    int8_t corrections[24] = {0, 0, 0, 0, 0, 1, 2, 3, 3, 3, 3, 3,
                              3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2};

    Battery() {
        batteryRelay.disable();
    }

    void loop() {

        expectedBedroomTemp = calcExpectedBedroomTemp();

        if (TemperatureSensors::isValidTemp(mqttUpdate.bedroomTemp)) {

            if (batteryRelay.isEnabled()) {
                if (mqttUpdate.bedroomTemp >= expectedBedroomTemp /*|| th.heatingHotTemp < th.batteryColdTemp*/) {
                    batteryRelay.disable();
                }
            } else {
                if (mqttUpdate.bedroomTemp <= expectedBedroomTemp - BORDER) {
                    batteryRelay.enable();
                }
            }
        }
    }

    float calcExpectedBedroomTemp() const {
        if (mqttUpdate.secondOfDay >= 0) {
            byte hourOfDay = static_cast<byte>(mqttUpdate.secondOfDay / 3600);
            return NORMAL_TEMP + corrections[hourOfDay];
        }
        return NORMAL_TEMP;
    }

    int getBatteryPompState() {
        return batteryRelay.isEnabled() ? 10 : 0;
    }

private:

    Relay batteryRelay = Relay(RELAY_BATTERY_POMP_PIN);

};

#endif
