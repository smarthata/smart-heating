#ifndef SMARTHATA_HEATING_MIXERRELAYS_H
#define SMARTHATA_HEATING_MIXERRELAYS_H

#include <Timeout.h>
#include <Relay.h>

class MixerRelays {
public:

    void run(int time) {
        if (time > 0) {
            up(static_cast<unsigned int>(time));
        } else {
            down(static_cast<unsigned int>(-time));
        }
    }

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
                relayMixerUp.disable();
            }
            if (relayMixerDown.isEnabled()) {
                relayMixerDown.disable();
            }
        }
    }

    void disable() {
        relayMixerUp.disable();
        relayMixerDown.disable();
    }

private:

    Relay relayMixerUp = Relay(RELAY_MIXER_UP_PIN);
    Relay relayMixerDown = Relay(RELAY_MIXER_DOWN_PIN);

    Timeout relayTimeout = Timeout();
};

#endif
