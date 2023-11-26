#include "platform/platform.h"

#include "rckid/rckid.h"
#include "rckid/config.h"
#include "rckid/serial.h"
#include "rckid/gpu/ST7789.h"
#include "rckid/gpu/canvas.h"
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

    ST7789::enterContinuousMode(320, 50);
    Canvas c{320, 50};
    c.setFg(Color::White());
    c.setFont(FreeMono12pt7b);
    //c.text("Hello world!", 0, 25);
    //c.pixel(0,0, Color::Blue());
    
    i2c_init(i2c0, 100000);
    gpio_set_function(RP_PIN_SDA, GPIO_FUNC_I2C);
    gpio_set_function(RP_PIN_SCL, GPIO_FUNC_I2C);
    //gpio_pull_up(RP_PIN_SDA);
    //gpio_pull_up(RP_PIN_SCL);    
    // Make the I2C pins available to picotool
    bi_decl(bi_2pins_with_func(RP_PIN_SDA, RP_PIN_SCL, GPIO_FUNC_I2C));  
    uint8_t rxd;
    for (uint8_t x = 0; x < 128; ++x) {
        if (i2c_read_blocking(i2c0, x, &rxd, 1, false) >= 0) {
            for (int y = 0; y < 20; ++y)
                c.pixel(x, y, Color::Red());
        }
    } 
    c.pixel(0,0, Color::White());
    ST7789::updateContinuous(c.rawPixels(), c.rawPixelsCount());

    /*
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
    */


    //PWM::initialize();
    while (true) {
        gpio::high(15);
        cpu::delayMs(100);
        gpio::low(15);
        cpu::delayMs(100);
    }
    return 0;
}