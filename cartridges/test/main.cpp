#include "platform/platform.h"

#include "serial.h"
#include "ST7789.h"

using namespace platform;

using namespace rckid;

int main() {
    Serial::initialize();
    ST7789::initialize();
    printf("Initialized\n");
    gpio::initialize();
    gpio::output(15);
    while (true) {
        gpio::high(15);
        cpu::delayMs(100);
        gpio::low(15);
        cpu::delayMs(100);
    }
    return 0;
}