#include <Arduino.h>
#include "Mixer.h"

Mixer *mixer;

void setup() {
    Serial.begin(115200);
    mixer = new Mixer();
}

void loop() {
    mixer->loop();
}
