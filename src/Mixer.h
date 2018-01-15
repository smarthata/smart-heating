#ifndef SMARTHATA_HEATING_MIXER_H
#define SMARTHATA_HEATING_MIXER_H

#include <DallasTemperature.h>
#include <Timing.h>
#include "Relay.h"

class Mixer {
public:
    static const int MIXER_CYCLE_TIME = 15000;
    static const int RELAY_ENABLE_TIME = 5000;

    static const int DALLAS_RESOLUTION = 11;

    static const int DALLAS_PIN = 4;
    static const int RELAY_MIXER_UP = 12;
    static const int RELAY_MIXER_DOWN = 11;

    Mixer() {
        pinMode(LED_BUILTIN, OUTPUT);
        dallasTemperature.begin();
        dallasTemperature.setResolution(DALLAS_RESOLUTION);
        printDevices();
        checkSensors();
    }

    void loop() {
        if (interval.isReady()) {

            updateTemperatures();

            if (mixedWaterTempC == -127.0) {
                Serial.println("mixedWaterSensor disconnected");
                blink();
                return;
            }

            if (mixedWaterTempC < temp - border) {
                Serial.println("UP");
                relayMixerUp.enable();
                delay(RELAY_ENABLE_TIME);
                relayMixerUp.disable();
            } else if (mixedWaterTempC > temp + border) {
                Serial.println("DOWN");
                relayMixerDown.enable();
                delay(RELAY_ENABLE_TIME);
                relayMixerDown.disable();
            } else {
                Serial.println("normal");
            }
        }
    }

private:
    const int temp = 30;
    const float border = 2.0;

    Relay relayMixerUp = Relay(RELAY_MIXER_UP);
    Relay relayMixerDown = Relay(RELAY_MIXER_DOWN);

    OneWire oneWire = OneWire(DALLAS_PIN);
    DallasTemperature dallasTemperature = DallasTemperature(&oneWire);

    DeviceAddress mixedWaterAddress = {0x28, 0x61, 0xBF, 0x3A, 0x06, 0x00, 0x00, 0x48};
    DeviceAddress coldWaterAddress = {0x28, 0x55, 0x8A, 0xCC, 0x06, 0x00, 0x00, 0x57};

    float mixedWaterTempC = 0;
    float coldWaterTempC = 0;

    Interval interval = Interval(MIXER_CYCLE_TIME);

    void updateTemperatures() {
        dallasTemperature.requestTemperatures();

        mixedWaterTempC = dallasTemperature.getTempC(mixedWaterAddress);
        coldWaterTempC = dallasTemperature.getTempC(coldWaterAddress);

        Serial.print("mixedWaterTempC = ");
        Serial.print(mixedWaterTempC);
        Serial.print(" \tcoldWaterTempC = ");
        Serial.print(coldWaterTempC);
        Serial.println();
    }

    void printDevices() {
        byte deviceCount = dallasTemperature.getDeviceCount();
        Serial.print("DallasTemperature deviceCount = ");
        Serial.println(deviceCount);

        for (int i = 0; i < deviceCount; ++i) {
            blink();
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

    void checkSensors() {
        Serial.print("mixedWaterSensor = ");
        Serial.println(dallasTemperature.isConnected(mixedWaterAddress));
        Serial.print("coldWaterSensor = ");
        Serial.println(dallasTemperature.isConnected(coldWaterAddress));

        if (!dallasTemperature.isConnected(mixedWaterAddress)) {
            Serial.println("mixedWaterSensor is not connected");
            error();
        }
    }

    void error() {
        Serial.println("Error");
        while (true) {
            blink();
        }
    }

    void blink() const {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(500);
        digitalWrite(LED_BUILTIN, LOW);
        delay(500);
    }
};

#endif
