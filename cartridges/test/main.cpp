#include "platform/platform.h"

#include "rckid/serial.h"
//#include "rckid/ST7789.h"
#include "rckid/gpu/ST7789.h"
#include "rckid/sd.h"

#include "rckid/apu/pwm.h"

using namespace platform;

using namespace rckid;

int main() {
    Serial::initialize();
    ST7789::initialize();
    printf("Initialized --test\n");
    //sd::test();
    gpio::initialize();
    gpio::output(15);

    ST7789::enterContinuousMode();
    Color * colors = new Color[320 * 10];
    for (uint8_t i = 0; ; ) {
    //for (int i = 0; i < 24; ++i) {
        for (int x = 0; x < 320 * 10; ++x)
            colors[x] = Color::RGB(i, 0, 0);
        ST7789::waitVSync();
        for (int x = 0; x < 24; ++x) {
            ST7789::waitUpdateDone();
            ST7789::updateContinuous(colors, 320*10);
        }
        i += 4;
        if (i == 0)
            break;
    }


    //PWM::initialize();
    while (true) {
        gpio::high(15);
        cpu::delayMs(100);
        gpio::low(15);
        cpu::delayMs(100);
    }
    return 0;
}