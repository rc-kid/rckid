#include "platform.h"

#include "bridge.h"





void setup() {
    // start serial protocol for debugging
    Serial.begin(115200);

    Bridge::initialize();
}   

void loop() {
    Serial.println("Tick");
    delay(1000);
}