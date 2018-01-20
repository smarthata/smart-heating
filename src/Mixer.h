#ifndef SMARTHATA_HEATING_MIXER_H
#define SMARTHATA_HEATING_MIXER_H

#include <Wire.h>
#include <DallasTemperature.h>
#include <TM1637Display.h>
#include <Timing.h>
#include "Relay.h"

void (*resetFunc)() = nullptr;

float temp = 25.0;
float mixedWaterTempC = 0;
float coldWaterTempC = 0;
float hotWaterTempC = 0;
float streetTempC = 0;

struct SmartHeatingDto {
    float tempFloor = 0;
    float realTemp = 0;
} dto;

class Mixer {
public:
    static const byte SMART_HEATING_I2C_ADDRESS = 15;

    static const int MIXER_CYCLE_TIME = 15000;
    static const int RELAY_ENABLE_TIME = 5000;

    static const int DALLAS_RESOLUTION = 11;

    static const int DALLAS_PIN = 4;
    static const int RELAY_MIXER_UP = 12;
    static const int RELAY_MIXER_DOWN = 11;

    Mixer() {
        pinMode(LED_BUILTIN, OUTPUT);

        initWire();

        dallasTemperature.begin();
        dallasTemperature.setResolution(DALLAS_RESOLUTION);
        printDevices();

        display.setBrightness(0x0f);
    }

    void loop() {
        if (readInterval.isReady()) {
            updateTemperatures();
            checkErrorStates();
        }

        if (interval.isReady()) {
            if (mixedWaterTempC < temp - border) {
                Serial.println("UP");
                relayMixerUp.enable();
                relayTimeout.start(RELAY_ENABLE_TIME);
            } else if (mixedWaterTempC > temp + border) {
                Serial.println("DOWN");
                relayMixerDown.enable();
                relayTimeout.start(RELAY_ENABLE_TIME);
            } else {
                Serial.println("normal");
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
    const float border = 0.5;

    Relay relayMixerUp = Relay(RELAY_MIXER_UP);
    Relay relayMixerDown = Relay(RELAY_MIXER_DOWN);

    TM1637Display display = TM1637Display(9, 10);

    OneWire oneWire = OneWire(DALLAS_PIN);
    DallasTemperature dallasTemperature = DallasTemperature(&oneWire);

    DeviceAddress mixedWaterAddress = {0x28, 0x61, 0xBF, 0x3A, 0x06, 0x00, 0x00, 0x48};
    DeviceAddress coldWaterAddress = {0x28, 0x55, 0x8A, 0xCC, 0x06, 0x00, 0x00, 0x57};
    DeviceAddress hotWaterAddress = {0x28, 0x6F, 0xE8, 0xCA, 0x06, 0x00, 0x00, 0xEE};
    DeviceAddress streetAddress = {0x28, 0xFF, 0x98, 0x3A, 0x91, 0x16, 0x04, 0x36};

    Interval interval = Interval(MIXER_CYCLE_TIME);
    Interval readInterval = Interval(1000);
    Interval relayInterval = Interval(100);
    Timeout relayTimeout;

    void updateTemperatures() {
        dallasTemperature.requestTemperatures();

        mixedWaterTempC = dallasTemperature.getTempC(mixedWaterAddress);
        coldWaterTempC = dallasTemperature.getTempC(coldWaterAddress);
        hotWaterTempC = dallasTemperature.getTempC(hotWaterAddress);
        streetTempC = dallasTemperature.getTempC(streetAddress);

        display.showNumberDec((int) (10 * mixedWaterTempC));

        Serial.print("temp = ");
        Serial.print(temp);
        Serial.print(" \tmixedWaterTempC = ");
        Serial.print(mixedWaterTempC);
//        Serial.print(" \tcoldWaterTempC = ");
//        Serial.print(coldWaterTempC);
//        Serial.print(" \thotWaterTempC = ");
//        Serial.print(hotWaterTempC);
//        Serial.print(" \tstreetTempC = ");
//        Serial.print(streetTempC);
        Serial.println();
    }

    void initWire() {
        Wire.begin(SMART_HEATING_I2C_ADDRESS);
        Wire.onReceive([](int size) {
            if (size != sizeof(SmartHeatingDto)) return;
            Wire.readBytes((char *) &dto, (size_t) size);
            temp = dto.tempFloor;
            Serial.println(temp);
        });
        Wire.onRequest([]() {
            dto.realTemp = mixedWaterTempC;
            Wire.write((char *) &dto, sizeof(SmartHeatingDto));
        });
    }

    void printDevices() {
        byte deviceCount = dallasTemperature.getDeviceCount();
        Serial.print("DallasTemperature deviceCount = ");
        Serial.println(deviceCount);

        for (int i = 0; i < deviceCount; ++i) {
            blink(300);
        }

        oneWire.reset_search();
        DeviceAddress tempAddress;
        while (oneWire.search(tempAddress)) {
            printAddress(tempAddress);
        }
    }

    void printAddress(DeviceAddress deviceAddress) {
        Serial.write('{');
        for (byte i = 0; i < 8; i++) {
            Serial.print("0x");
            if (deviceAddress[i] < 16) Serial.print("0");
            Serial.print(deviceAddress[i], HEX);
            Serial.print(", ");
        }
        Serial.println("}");
    }

    void checkErrorStates() {
        if (mixedWaterTempC == DEVICE_DISCONNECTED_C) {
            Serial.println("mixedWaterSensor disconnected");
            error();
        }
//        if (coldWaterTempC == DEVICE_DISCONNECTED_C) {
//            Serial.println("coldWaterSensor disconnected");
//            blink();
//        }
//        if (hotWaterTempC == DEVICE_DISCONNECTED_C) {
//            Serial.println("hotWaterTempC disconnected");
//            blink();
//        }
//        if (streetTempC == DEVICE_DISCONNECTED_C) {
//            Serial.println("streetTempC disconnected");
//            blink();
//        }
//        if (coldWaterTempC > hotWaterTempC) {
//            Serial.println("coldWaterTempC > hotWaterTempC");
//            blink();
//        }
    }

    void error() {
        relayMixerUp.disable();
        relayMixerDown.disable();

        Serial.println("Error");

        for (int i = 0; i < 10; ++i) {
            blink(1000);
        }
        Serial.println("Reset");
        Serial.flush();
        resetFunc();
    }

    void blink(unsigned int delayMs = 500) const {
        Serial.write('.');
        digitalWrite(LED_BUILTIN, HIGH);
        delay(delayMs);
        digitalWrite(LED_BUILTIN, LOW);
        delay(delayMs);
    }
};

#endif
