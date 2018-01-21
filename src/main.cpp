#include <Arduino.h>

//#define DEBUG true
#define DISPLAY_SSD1306 true

#ifdef DEBUG
#define DEBUG_SERIAL(var) Serial.print(var);
#define DEBUG_SERIAL_F(var) Serial.print(F(var));
#define DEBUG_SERIAL_HEX(var, base) Serial.print(var, base);
#define DEBUG_SERIAL_LN(var) Serial.println(var);
#define DEBUG_SERIAL_LN_F(var) Serial.println(F(var));
#else
#define DEBUG_SERIAL(var)
#define DEBUG_SERIAL_F(var)
#define DEBUG_SERIAL_HEX(var, base)
#define DEBUG_SERIAL_LN(var)
#define DEBUG_SERIAL_LN_F(var)
#endif

#include "Mixer.h"

Mixer *mixer;

void setup() {
#ifdef DEBUG
    Serial.begin(115200);
#endif
    mixer = new Mixer();
}

void loop() {
    mixer->loop();
}
