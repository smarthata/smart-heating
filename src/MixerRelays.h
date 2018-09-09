#ifndef SMARTHATA_HEATING_MIXERRELAYS_H
#define SMARTHATA_HEATING_MIXERRELAYS_H

#include <Interval.h>
#include <Timeout.h>
#include "Relay.h"

class MixerRelays {
public:

    void up(unsigned int timeEnabled) {
        relayMixerDown.disable();
        relayMixerUp.enable();
        relayTimeout.start(timeEnabled);
    }

    void down(unsigned int timeEnabled) {
        relayMixerUp.disable();
        relayMixerDown.enable();
        relayTimeout.start(timeEnabled);
    }

    void loop() {
        if (relayTimeout.isReady()) {
            if (relayMixerUp.isEnabled()) {
                Serial.println("disable UP");
                relayMixerUp.disable();
            }
            if (relayMixerDown.isEnabled()) {
                Serial.println("disable DOWN");
                relayMixerDown.disable();
            }
        }
    }

    void disable() {
        relayMixerUp.disable();
        relayMixerDown.disable();
    }

private:
    static const int RELAY_MIXER_UP = 12;
    static const int RELAY_MIXER_DOWN = 11;

    Relay relayMixerUp = Relay(RELAY_MIXER_UP);
    Relay relayMixerDown = Relay(RELAY_MIXER_DOWN);

    Timeout relayTimeout = Timeout();
};

#endif
