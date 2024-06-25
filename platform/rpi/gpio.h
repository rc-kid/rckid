#pragma once

class gpio {
public:
    using Pin = unsigned;
    static constexpr Pin UNUSED = 0xffffffff;

    enum class Edge {
        Rising = INT_EDGE_RISING, 
        Falling = INT_EDGE_FALLING, 
        Both = INT_EDGE_BOTH,
    }; 

    static void initialize() {
        wiringPiSetupGpio();
    }

    static void setAsOutput(Pin pin) {
        pinMode(pin, OUTPUT);
    }

    static void setAsInput(Pin pin) {
        pinMode(pin, INPUT);
        pullUpDnControl(pin, PUD_OFF);
    }

    static void setAsInputPullup(Pin pin) {
        pinMode(pin, INPUT);
        pullUpDnControl(pin, PUD_UP);
    }

    static void write(Pin pin, bool value) {
        digitalWrite(pin, value ? HIGH : LOW);
    }

    static bool read(Pin pin) { 
        return digitalRead(pin) == HIGH;
    }

    static void attachInterrupt(Pin pin, Edge edge, void(*handler)()) {
        wiringPiISR(pin, (int) edge, handler);
    } 
}; // gpio
