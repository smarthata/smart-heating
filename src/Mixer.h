#ifndef SMARTHATA_HEATING_MIXER_H
#define SMARTHATA_HEATING_MIXER_H

#include <Wire.h>
#include <DallasTemperature.h>
#include <Adafruit_SSD1306.h>
#include <Timing.h>
#include "Relay.h"

void (*resetFunc)() = nullptr;

struct SmartHeatingDto {
    float tempFloor = 0;
    float mixedWaterTemp = 0;
    float coldWaterTemp = 0;
    float hotWaterTemp = 0;
    float streetTemp = 0;
} th;

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
        }

        if (interval.isReady()) {
            if (th.mixedWaterTemp < th.tempFloor - border) {
                DEBUG_SERIAL_LN_F("UP");
                relayMixerUp.enable();
                relayTimeout.start(RELAY_ENABLE_TIME);
            } else if (th.mixedWaterTemp > th.tempFloor + border) {
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

        th.mixedWaterTemp = safeReadTemp(mixedWaterAddress);
        th.coldWaterTemp = safeReadTemp(coldWaterAddress);
        th.hotWaterTemp = safeReadTemp(hotWaterAddress);
        th.streetTemp = safeReadTemp(streetAddress);

#ifdef DISPLAY_SSD1306
        display.clearDisplay();
        displayTemp(0, 0, th.tempFloor);
        displayTemp(0, 16, th.mixedWaterTemp);
        displayTemp(0, 32, th.hotWaterTemp);
        displayTemp(0, 48, th.coldWaterTemp);
        display.display();
#endif
        DEBUG_SERIAL_F("tempFloor = ");
        DEBUG_SERIAL(th.tempFloor);
        DEBUG_SERIAL_F(" \tmixedWaterTemp = ");
        DEBUG_SERIAL(th.mixedWaterTemp);
        DEBUG_SERIAL_F(" \tcoldWaterTemp = ");
        DEBUG_SERIAL(th.coldWaterTemp);
        DEBUG_SERIAL_F(" \thotWaterTemp = ");
        DEBUG_SERIAL(th.hotWaterTemp);
        DEBUG_SERIAL_F(" \tstreetTemp = ");
        DEBUG_SERIAL(th.streetTemp);
        DEBUG_SERIAL_LN();
    }
    
    float safeReadTemp(DeviceAddress &address) {
        float tempC = dallasTemperature.getTempC(address);
        Timeout t;
        t.start(1000);
        while (tempC == DEVICE_DISCONNECTED_C && !t.isReady()) {
            dallasTemperature.requestTemperaturesByAddress(address);
            tempC = dallasTemperature.getTempC(address);
        }
        return tempC;
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

            SmartHeatingDto dto;
            Wire.readBytes((char *) &dto, (size_t) size);

            if (dto.tempFloor >= 10 && dto.tempFloor <= 35) {
                th.tempFloor = dto.tempFloor;
                DEBUG_SERIAL_LN(th.tempFloor);
            }
        });
        Wire.onRequest([]() {
            SmartHeatingDto dto;
            dto.mixedWaterTemp = th.mixedWaterTemp;
            dto.hotWaterTemp = th.hotWaterTemp;
            dto.coldWaterTemp = th.coldWaterTemp;
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
