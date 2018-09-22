#include <Arduino.h>
#include "config.h"
#include "SmarthataHeating.h"

SmarthataHeating *heating;

void setup() {
    Serial.begin(115200);
    heating = new SmarthataHeating(ssid, pass);
}

void loop() {
    heating->loop();
}
