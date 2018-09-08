#ifndef SMARTHATA_HEATING_MIXER_H
#define SMARTHATA_HEATING_MIXER_H

#include <Wire.h>
#include <DallasTemperature.h>
#include <Timeout.h>
#include <Interval.h>
#include "Relay.h"

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
    static const int RELAY_MIXER_UP = 12;
    static const int RELAY_MIXER_DOWN = 11;

    Mixer() {
        pinMode(LED_BUILTIN, OUTPUT);

        th.floorTemp = 25;

        initWire();
        initTemperatureSensors();
    }

    void loop() {
        if (readInterval.isReady()) {
            updateTemperatures();
        }

        if (interval.isReady() && th.floorMixedTemp != DEVICE_DISCONNECTED_C) {
            float floorMediumTemp = th.floorMixedTemp;
            if(th.floorColdTemp != DEVICE_DISCONNECTED_C)
                floorMediumTemp = (th.floorMixedTemp + th.floorColdTemp) * 0.5;
            if (floorMediumTemp < th.floorTemp - border) {
                DEBUG_SERIAL_LN_F("UP");
                relayMixerUp.enable();

                float diff = constrain(th.floorTemp - border - floorMediumTemp, border, 2);
                relayTime = calcRelayTime(diff);
                relayTimeout.start(relayTime);
            } else if (floorMediumTemp > th.floorTemp + border) {
                DEBUG_SERIAL_LN_F("DOWN");
                relayMixerDown.enable();

                float diff = constrain(floorMediumTemp - th.floorTemp - border, border, 2);
                relayTime = calcRelayTime(diff);
                relayTimeout.start(relayTime);
            } else {
                relayTime = 0;
                DEBUG_SERIAL_LN_F("normal");
            }
        }

        if (relayInterval.isReady() && relayTimeout.isReady()) {
            if (relayMixerUp.isEnabled()) {
                relayMixerUp.disable();
            } else if (relayMixerDown.isEnabled()) {
                relayMixerDown.disable();
            }
        }
    }

private:
    const float border = 0.1;

    Relay relayMixerUp = Relay(RELAY_MIXER_UP);
    Relay relayMixerDown = Relay(RELAY_MIXER_DOWN);

    OneWire oneWire = OneWire(DALLAS_PIN);
    DallasTemperature dallasTemperature = DallasTemperature(&oneWire);

    DeviceAddress mixedWaterAddress = {0x28, 0x61, 0xBF, 0x3A, 0x06, 0x00, 0x00, 0x48};
    DeviceAddress coldWaterAddress = {0x28, 0x55, 0x8A, 0xCC, 0x06, 0x00, 0x00, 0x57};
    DeviceAddress hotWaterAddress = {0x28, 0x6F, 0xE8, 0xCA, 0x06, 0x00, 0x00, 0xEE};
    DeviceAddress batteryColdAddress = {0x28, 0xC2, 0x6E, 0xCB, 0x06, 0x00, 0x00, 0x20};
    DeviceAddress boilerAddress = {0x28, 0xD4, 0xD3, 0xE1, 0x06, 0x00, 0x00, 0x01};
    DeviceAddress streetAddress = {0x28, 0xFF, 0x98, 0x3A, 0x91, 0x16, 0x04, 0x36};

    Timeout readTimeout;

    Interval interval = Interval(MIXER_CYCLE_TIME);
    Interval readInterval = Interval(1000);
    Interval relayInterval = Interval(100);
    Timeout relayTimeout;
    unsigned int relayTime = 0;

    void updateTemperatures() {
        dallasTemperature.requestTemperatures();

        th.floorMixedTemp = safeReadTemp(mixedWaterAddress);
        th.floorColdTemp = safeReadTemp(coldWaterAddress);
        th.heatingHotTemp = safeReadTemp(hotWaterAddress);
        th.batteryColdTemp = safeReadTemp(batteryColdAddress);
        th.boilerTemp = safeReadTemp(boilerAddress);
        th.streetTemp = safeReadTemp(streetAddress);

        DEBUG_SERIAL_F("floorTemp = ");
        DEBUG_SERIAL(th.floorTemp);
        DEBUG_SERIAL_F(" \tfloorMixed = ");
        DEBUG_SERIAL(th.floorMixedTemp);
        DEBUG_SERIAL_F(" \tfloorCold = ");
        DEBUG_SERIAL(th.floorColdTemp);
        DEBUG_SERIAL_F(" \theatingHot = ");
        DEBUG_SERIAL(th.heatingHotTemp);
        DEBUG_SERIAL_F(" \tbatteryCold = ");
        DEBUG_SERIAL(th.batteryColdTemp);
        DEBUG_SERIAL_F(" \tboiler = ");
        DEBUG_SERIAL(th.boilerTemp);
        DEBUG_SERIAL_F(" \tstreet = ");
        DEBUG_SERIAL(th.streetTemp);
        DEBUG_SERIAL_LN();
    }

    float safeReadTemp(DeviceAddress &address) {
        float tempC = dallasTemperature.getTempC(address);
        readTimeout.start(2000);
        while (tempC == DEVICE_DISCONNECTED_C && !readTimeout.isReady()) {
            dallasTemperature.requestTemperaturesByAddress(address);
            tempC = dallasTemperature.getTempC(address);
        }
        return tempC;
    }

    unsigned int calcRelayTime(float diff) const {
        return (unsigned int) mapFloat(diff, border, 1.5, 1000, 8000);
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
                DEBUG_SERIAL_LN(th.floorTemp);
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
        DEBUG_SERIAL_F("DallasTemperature deviceCount = ");
        DEBUG_SERIAL_LN(deviceCount);

        for (int i = 0; i < deviceCount; ++i) {
            blink(300);
        }

#ifdef DEBUG
        oneWire.reset_search();
        DeviceAddress tempAddress;
        while (oneWire.search(tempAddress)) {
            printAddress(tempAddress);
        }
#endif
    }

    void printAddress(DeviceAddress deviceAddress) {
        DEBUG_SERIAL_F("{");
        for (byte i = 0; i < 8; i++) {
            DEBUG_SERIAL_F("0x");
            if (deviceAddress[i] < 16) DEBUG_SERIAL_F("0");
            DEBUG_SERIAL_HEX(deviceAddress[i], HEX);
            DEBUG_SERIAL_F(", ");
        }
        DEBUG_SERIAL_LN_F("}");
    }

    void error() {
        relayMixerUp.disable();
        relayMixerDown.disable();

        DEBUG_SERIAL_LN_F("Error");

        for (int i = 0; i < 10; ++i) {
            blink(1000);
        }
        DEBUG_SERIAL_LN_F("Reset");
#ifdef DEBUG
        Serial.flush();
#endif
        resetFunc();
    }

    void blink(unsigned int delayMs = 500) const {
        DEBUG_SERIAL_F(".");
        digitalWrite(LED_BUILTIN, HIGH);
        delay(delayMs);
        digitalWrite(LED_BUILTIN, LOW);
        delay(delayMs);
    }
};

#endif
