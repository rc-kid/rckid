#include "platform/platform.h"

using namespace platform;

int main() {
    gpio::initialize();
    gpio::output(25);
    while (true) {
        gpio::high(25);
        cpu::delayMs(100);
        gpio::low(25);
        cpu::delayMs(100);
    }
    return 0;
}