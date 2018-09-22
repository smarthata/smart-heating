#ifndef SMARTHATA_HEATING_TEMPERATURESENSORS_H
#define SMARTHATA_HEATING_TEMPERATURESENSORS_H

#include <DallasTemperature.h>
#include <Timeout.h>

struct SmartHeatingDto {
    float floorMixedTemp = 0;
    float floorColdTemp = 0;
    float heatingHotTemp = 0;
    float batteryColdTemp = 0;
    float boilerTemp = 0;
    float streetTemp = 0;
};

class TemperatureSensors {
private:
    static const int DALLAS_RESOLUTION = 12;

    OneWire oneWire = OneWire(DALLAS_PIN);
    DallasTemperature dallasTemperature = DallasTemperature(&oneWire);

    SmartHeatingDto th;

    DeviceAddress mixedWaterAddress = {0x28, 0x61, 0xBF, 0x3A, 0x06, 0x00, 0x00, 0x48};
    DeviceAddress coldWaterAddress = {0x28, 0x55, 0x8A, 0xCC, 0x06, 0x00, 0x00, 0x57};
    DeviceAddress hotWaterAddress = {0x28, 0x6F, 0xE8, 0xCA, 0x06, 0x00, 0x00, 0xEE};
    DeviceAddress batteryColdAddress = {0x28, 0xC2, 0x6E, 0xCB, 0x06, 0x00, 0x00, 0x20};
    DeviceAddress boilerAddress = {0x28, 0xD4, 0xD3, 0xE1, 0x06, 0x00, 0x00, 0x01};
    DeviceAddress streetAddress = {0x28, 0xFF, 0x98, 0x3A, 0x91, 0x16, 0x04, 0x36};

public:
    TemperatureSensors() {
        dallasTemperature.begin();
        dallasTemperature.setResolution(DALLAS_RESOLUTION);
        printDevices();
    }

    SmartHeatingDto updateTemperatures() {
        dallasTemperature.requestTemperatures();

        th.floorMixedTemp = safeReadTemp(mixedWaterAddress);
        th.floorColdTemp = safeReadTemp(coldWaterAddress);
        th.heatingHotTemp = safeReadTemp(hotWaterAddress);
        th.batteryColdTemp = safeReadTemp(batteryColdAddress);
        th.boilerTemp = safeReadTemp(boilerAddress);
        th.streetTemp = safeReadTemp(streetAddress);

        Serial.print("Read temperatures: ");
        printValue("floorMixedTemp", th.floorMixedTemp);
        printValue("floorColdTemp", th.floorColdTemp);
        printValue("heatingHotTemp", th.heatingHotTemp);
        printValue("batteryColdTemp", th.batteryColdTemp);
        printValue("boilerTemp", th.boilerTemp);
        printValue("streetTemp", th.streetTemp);
        Serial.println();

        return th;
    }

    bool isValidTemp(const float tempC) const {
        return !isDisconnected(tempC) && fabs(tempC) < 50;
    }

    float calcFloorMediumTemp(const SmartHeatingDto &th) const {
        float floorMediumTemp = th.floorMixedTemp;
        if (isValidTemp(th.floorColdTemp))
            floorMediumTemp = (th.floorMixedTemp + th.floorColdTemp) * 0.5f;
        return floorMediumTemp;
    }

private:

    bool isDisconnected(const float tempC) const {
        return tempC == DEVICE_DISCONNECTED_C;
    }

    void printValue(const char *name, float value) const {
        Serial.print(String(name) + " = " + value + " \t");
    }

    float safeReadTemp(DeviceAddress &address) {
        float tempC = dallasTemperature.getTempC(address);
        Timeout readTimeout(1000);
        while (tempC == DEVICE_DISCONNECTED_C && !readTimeout.isReady()) {
            dallasTemperature.requestTemperaturesByAddress(address);
            tempC = dallasTemperature.getTempC(address);
            delay(100);
        }
        return tempC;
    }

    void printDevices() {
        byte deviceCount = dallasTemperature.getDeviceCount();
        Serial.print("DallasTemperature deviceCount = ");
        Serial.println(deviceCount);

        for (int i = 0; i < deviceCount; ++i) {
            blink(300);
        }
        Serial.println();

        oneWire.reset_search();
        DeviceAddress tempAddress;
        while (oneWire.search(tempAddress)) {
            printAddress(tempAddress);
        }
    }

    void printAddress(DeviceAddress deviceAddress) {
        Serial.print("{");
        for (byte i = 0; i < 8; i++) {
            Serial.print("0x");
            if (deviceAddress[i] < 16) Serial.print("0");
            Serial.print(deviceAddress[i], HEX);
            Serial.print(", ");
        }
        Serial.println("}");
    }

    void blink(unsigned int delayMs = 300) const {
        Serial.print(".");
        digitalWrite(LED_BUILTIN, HIGH);
        delay(delayMs);
        digitalWrite(LED_BUILTIN, LOW);
        delay(delayMs);
    }
};

#endif
