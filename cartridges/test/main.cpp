#include "platform/platform.h"

#include "rckid/serial.h"
#include "rckid/ST7789.h"
#include "rckid/sd.h"

using namespace platform;

using namespace rckid;

int main() {
    Serial::initialize();
    ST7789::initialize();
    printf("Initialized --test\n");
    sd::test();
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