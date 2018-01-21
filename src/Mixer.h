#ifndef SMARTHATA_HEATING_MIXER_H
#define SMARTHATA_HEATING_MIXER_H

#include <Wire.h>
#include <DallasTemperature.h>
#include <Adafruit_SSD1306.h>
#include <Timing.h>
#include "Relay.h"

void (*resetFunc)() = nullptr;

float temp = 24.0;
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
    static const byte DISPLAY_SSD1306_ADDRESS = 0x3C;

    static const int MIXER_CYCLE_TIME = 15000;
    static const int RELAY_ENABLE_TIME = 5000;

    static const int DALLAS_RESOLUTION = 11;

    static const int DALLAS_PIN = 4;
    static const int RELAY_MIXER_UP = 12;
    static const int RELAY_MIXER_DOWN = 11;

    Mixer() {
        pinMode(LED_BUILTIN, OUTPUT);

        initWire();
        initTemperatureSensors();
        initDisplay();
    }

    void loop() {
        if (readInterval.isReady()) {
            updateTemperatures();
            checkErrorStates();
        }

        if (interval.isReady()) {
            if (mixedWaterTempC < temp - border) {
                DEBUG_SERIAL_LN_F("UP");
                relayMixerUp.enable();
                relayTimeout.start(RELAY_ENABLE_TIME);
            } else if (mixedWaterTempC > temp + border) {
                DEBUG_SERIAL_LN_F("DOWN");
                relayMixerDown.enable();
                relayTimeout.start(RELAY_ENABLE_TIME);
            } else {
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
    const float border = 0.5;

    Relay relayMixerUp = Relay(RELAY_MIXER_UP);
    Relay relayMixerDown = Relay(RELAY_MIXER_DOWN);

#ifdef DISPLAY_SSD1306
    Adafruit_SSD1306 display;
#endif

    OneWire oneWire = OneWire(DALLAS_PIN);
    DallasTemperature dallasTemperature = DallasTemperature(&oneWire);

    DeviceAddress mixedWaterAddress = {0x28, 0x61, 0xBF, 0x3A, 0x06, 0x00, 0x00, 0x48};
    DeviceAddress coldWaterAddress = {0x28, 0x55, 0x8A, 0xCC, 0x06, 0x00, 0x00, 0x57};
    DeviceAddress hotWaterAddress = {0x28, 0x6F, 0xE8, 0xCA, 0x06, 0x00, 0x00, 0xEE};
    DeviceAddress streetAddress = {0x28, 0xFF, 0x98, 0x3A, 0x91, 0x16, 0x04, 0x36};

    Interval interval = Interval(MIXER_CYCLE_TIME);
    Interval readInterval = Interval(3000);
    Interval relayInterval = Interval(100);
    Timeout relayTimeout;

    void updateTemperatures() {
        dallasTemperature.requestTemperatures();

        mixedWaterTempC = dallasTemperature.getTempC(mixedWaterAddress);
        coldWaterTempC = dallasTemperature.getTempC(coldWaterAddress);
        hotWaterTempC = dallasTemperature.getTempC(hotWaterAddress);
        streetTempC = dallasTemperature.getTempC(streetAddress);

#ifdef DISPLAY_SSD1306
        display.clearDisplay();
        displayTemp(0, 0, temp);
        displayTemp(0, 16, mixedWaterTempC);
        displayTemp(0, 32, hotWaterTempC);
        displayTemp(0, 48, coldWaterTempC);
        display.display();
#endif
        DEBUG_SERIAL_F("temp = ");
        DEBUG_SERIAL(temp);
        DEBUG_SERIAL_F(" \tmixedWaterTempC = ");
        DEBUG_SERIAL(mixedWaterTempC);
        DEBUG_SERIAL_F(" \tcoldWaterTempC = ");
        DEBUG_SERIAL(coldWaterTempC);
        DEBUG_SERIAL_F(" \thotWaterTempC = ");
        DEBUG_SERIAL(hotWaterTempC);
//        DEBUG_SERIAL_F(" \tstreetTempC = ");
//        DEBUG_SERIAL(streetTempC);
        DEBUG_SERIAL_LN();
    }

    void displayTemp(int x, int y, float t) {
#ifdef DISPLAY_SSD1306
        display.setCursor(x, y);
        display.print(t, 1);
        display.print((char) 247);
        display.print("C");
#endif
    }

    void initWire() {
        Wire.begin(SMART_HEATING_I2C_ADDRESS);
        Wire.onReceive([](int size) {
            if (size != sizeof(SmartHeatingDto)) return;
            Wire.readBytes((char *) &dto, (size_t) size);
            temp = dto.tempFloor;
            DEBUG_SERIAL_LN(temp);
        });
        Wire.onRequest([]() {
            dto.realTemp = mixedWaterTempC;
            Wire.write((char *) &dto, sizeof(SmartHeatingDto));
        });
    }

    void initTemperatureSensors() {
        dallasTemperature.begin();
        dallasTemperature.setResolution(DALLAS_RESOLUTION);
        printDevices();
    }


    void initDisplay() {
#ifdef DISPLAY_SSD1306
        display.begin(SSD1306_SWITCHCAPVCC, DISPLAY_SSD1306_ADDRESS);
        display.clearDisplay();
        display.display();

        display.setTextColor(WHITE);
        display.setTextSize(2);
#endif
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

    void checkErrorStates() {
        if (mixedWaterTempC == DEVICE_DISCONNECTED_C) {
            DEBUG_SERIAL_LN_F("mixedWaterSensor disconnected");
//            error();
        }
//        if (coldWaterTempC == DEVICE_DISCONNECTED_C) {
//            DEBUG_SERIAL_LN_F("coldWaterSensor disconnected");
//            blink();
//        }
//        if (hotWaterTempC == DEVICE_DISCONNECTED_C) {
//            DEBUG_SERIAL_LN_F("hotWaterTempC disconnected");
//            blink();
//        }
//        if (streetTempC == DEVICE_DISCONNECTED_C) {
//            DEBUG_SERIAL_LN_F("streetTempC disconnected");
//            blink();
//        }
//        if (coldWaterTempC > hotWaterTempC) {
//            DEBUG_SERIAL_LN_F("coldWaterTempC > hotWaterTempC");
//            blink();
//        }
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
