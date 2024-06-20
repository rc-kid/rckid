#include "platform.h"

void setup() {
    Serial.begin(9600);
}

void loop() {
    Serial.println("Tick");
    delay(1000);
}