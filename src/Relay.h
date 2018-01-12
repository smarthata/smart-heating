#ifndef SMARTHATA_HEATING_RELAY_H
#define SMARTHATA_HEATING_RELAY_H

class Relay {
public:
    Relay(const byte pin, const byte enabledVoltage = HIGH) :
            pin(pin), enabledVoltage(enabledVoltage) {
        pinMode(pin, OUTPUT);
        disable();
    }

    void enable() {
        digitalWrite(pin, enabledVoltage);
        enabled = true;
    }

    void disable() {
        digitalWrite(pin, HIGH - enabledVoltage);
        enabled = false;
    }

    bool isEnabled() const {
        return enabled;
    }

    const byte getPin() const {
        return pin;
    }

private:
    const byte pin{};
    const byte enabledVoltage{};
    bool enabled = false;
};

#endif
