#ifndef SMARTHATA_HEATING_MIXER_H
#define SMARTHATA_HEATING_MIXER_H

#include <Wire.h>
#include <DallasTemperature.h>
#include <Timeout.h>
#include <Interval.h>
#include "MixerRelays.h"

void (*resetFunc)() = nullptr;

struct SmartHeatingDto {
    float floorTemp = 0;
    float floorMixedTemp = 0;
    float floorColdTemp = 0;
    float heatingHotTemp = 0;
    float batteryColdTemp = 0;
    float boilerTemp = 0;
    float streetTemp = 0;
} th;

class Mixer {
public:
    static const byte SMART_HEATING_I2C_ADDRESS = 15;

    static const int MIXER_CYCLE_TIME = 10000;

    static const int DALLAS_RESOLUTION = 12;

    static const int DALLAS_PIN = 4;


    Mixer() {
        pinMode(LED_BUILTIN, OUTPUT);

        th.floorTemp = 25;

        initWire();
        initTemperatureSensors();
        interval.startWithCurrentTimeEnabled();
        readInterval.startWithCurrentTimeEnabled();
    }

    void loop() {
        if (readInterval.isReady()) {
            updateTemperatures();
        }

        if (th.floorMixedTemp != DEVICE_DISCONNECTED_C && interval.isReady()) {
            float floorMediumTemp = th.floorMixedTemp;
            if (th.floorColdTemp != DEVICE_DISCONNECTED_C)
                floorMediumTemp = (th.floorMixedTemp + th.floorColdTemp) * 0.5;
            if (floorMediumTemp < th.floorTemp - BORDER) {
                float diff = constrain(th.floorTemp - BORDER - floorMediumTemp, BORDER, 2);
                unsigned int time = calcRelayTime(diff);
                Serial.println(String("UP ") + time + " ms");
                mixerRelays.up(time);
            } else if (floorMediumTemp > th.floorTemp + BORDER) {
                float diff = constrain(floorMediumTemp - th.floorTemp - BORDER, BORDER, 2);
                unsigned int time = calcRelayTime(diff);
                Serial.println(String("DOWN ") + time + " ms");
                mixerRelays.down(time);
            } else {
                Serial.println("normal");
            }
        }

        mixerRelays.loop();
    }

private:
    static constexpr float BORDER = 0.1;

    MixerRelays mixerRelays = MixerRelays();

    OneWire oneWire = OneWire(DALLAS_PIN);
    DallasTemperature dallasTemperature = DallasTemperature(&oneWire);

    DeviceAddress mixedWaterAddress = {0x28, 0x61, 0xBF, 0x3A, 0x06, 0x00, 0x00, 0x48};
    DeviceAddress coldWaterAddress = {0x28, 0x55, 0x8A, 0xCC, 0x06, 0x00, 0x00, 0x57};
    DeviceAddress hotWaterAddress = {0x28, 0x6F, 0xE8, 0xCA, 0x06, 0x00, 0x00, 0xEE};
    DeviceAddress batteryColdAddress = {0x28, 0xC2, 0x6E, 0xCB, 0x06, 0x00, 0x00, 0x20};
    DeviceAddress boilerAddress = {0x28, 0xD4, 0xD3, 0xE1, 0x06, 0x00, 0x00, 0x01};
    DeviceAddress streetAddress = {0x28, 0xFF, 0x98, 0x3A, 0x91, 0x16, 0x04, 0x36};

    Interval interval = Interval(MIXER_CYCLE_TIME);
    Interval readInterval = Interval(100 0);

    void updateTemperatures() {
        dallasTemperature.requestTemperatures();

        th.floorMixedTemp = safeReadTemp(mixedWaterAddress);
        th.floorColdTemp = safeReadTemp(coldWaterAddress);
        th.heatingHotTemp = safeReadTemp(hotWaterAddress);
        th.batteryColdTemp = safeReadTemp(batteryColdAddress);
        th.boilerTemp = safeReadTemp(boilerAddress);
        th.streetTemp = safeReadTemp(streetAddress);

        printValue("floorTemp", th.floorTemp);
        printValue("floorMixedTemp", th.floorMixedTemp);
        printValue("floorColdTemp", th.floorColdTemp);
        printValue("heatingHotTemp", th.heatingHotTemp);
        printValue("batteryColdTemp", th.batteryColdTemp);
        printValue("boilerTemp", th.boilerTemp);
        printValue("streetTemp", th.streetTemp);
        Serial.println();
    }

    void printValue(const char *name, float value) const {
        Serial.print(String(name) + " = " + value + " \t");
    }

    float safeReadTemp(DeviceAddress &address) {
        float tempC = dallasTemperature.getTempC(address);
        Timeout readTimeout;
        readTimeout.start(1000);
        while (tempC == DEVICE_DISCONNECTED_C && !readTimeout.isReady()) {
            dallasTemperature.requestTemperaturesByAddress(address);
            tempC = dallasTemperature.getTempC(address);
        }
        return tempC;
    }

    unsigned int calcRelayTime(float diff) const {
        return (unsigned int) mapFloat(diff, BORDER, 2, 1000, 8000);
    }

    float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) const {
        return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
    }

    void initWire() {
        Wire.begin(SMART_HEATING_I2C_ADDRESS);
        Wire.onReceive([](int size) {
            if (size != sizeof(SmartHeatingDto)) return;

            SmartHeatingDto dto;
            Wire.readBytes((char *) &dto, (size_t) size);

            if (dto.floorTemp >= 10 && dto.floorTemp <= 45) {
                th.floorTemp = dto.floorTemp;
                Serial.println(th.floorTemp);
            }
        });
        Wire.onRequest([]() {
            SmartHeatingDto dto;
            dto.floorTemp = th.floorTemp;
            dto.floorMixedTemp = th.floorMixedTemp;
            dto.floorColdTemp = th.floorColdTemp;
            dto.heatingHotTemp = th.heatingHotTemp;
            dto.batteryColdTemp = th.batteryColdTemp;
            dto.boilerTemp = th.boilerTemp;
            dto.streetTemp = th.streetTemp;
            Wire.write((char *) &dto, sizeof(SmartHeatingDto));
        });
    }

    void initTemperatureSensors() {
        dallasTemperature.begin();
        dallasTemperature.setResolution(DALLAS_RESOLUTION);
        printDevices();
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

    void error() {
        Serial.println("Error. RESET");
        Serial.flush();
        mixerRelays.disable();
        for (int i = 0; i < 10; ++i) {
            blink(1000);
        }
        resetFunc();
    }

    void blink(unsigned int delayMs = 500) const {
        Serial.print(".");
        digitalWrite(LED_BUILTIN, HIGH);
        delay(delayMs);
        digitalWrite(LED_BUILTIN, LOW);
        delay(delayMs);
    }
};

#endif
