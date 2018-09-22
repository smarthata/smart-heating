#include <Arduino.h>
#include "config.h"
#include "Mixer.h"

Mixer *mixer;

void setup() {
    Serial.begin(115200);
    mixer = new Mixer(ssid, pass);
}

void loop() {
    mixer->loop();
}
