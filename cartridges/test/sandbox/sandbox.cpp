#include "rckid/app.h"
#include "rckid/stats.h"
#include "rckid/graphics/framebuffer.h"
#include "rckid/graphics/font.h"
#include "assets/fonts/IosevkaNF_16.h"

using namespace rckid;


constexpr Font Iosevka16 = Font::fromROM<IosevkaNF_20>();

class SimpleApp : public App<FrameBuffer<ColorRGB>> {
public:

protected:

    void update() override {

    }

    void draw() override {
        driver_.fill();
        driver_.text(0, 0, Iosevka16, Color::White()) << "Hello world! (font)";
        driver_.text(0, 64) << "Hello world! (gfx)";
    }



}; // SimpleApp

int main() {
    rckid::initialize();
    SimpleApp{}.run();
   /*
    cpu::overclock();
    Menu m{{
        MenuItem::create("AVR Status", assets::Gameboy), 
        MenuItem::create("Sensors", assets::Gameboy), 
        MenuItem::create("Rumbler", assets::Gameboy),
        MenuItem::create("Sliding Puzzle", assets::Gameboy), 
    }};
    Carousel c{&m};
    while (true) {
        TRACE("Starting carousel");
        auto r = c.run();
        if (r.has_value()) {
            switch (r.value()) {
                case 0:
                    TRACE("Starting AVR status");
                    AVRStatusTest{}.run();
                    break;
                case 1:
                    TRACE("Starting sensors");
                    SensorsTest{}.run();
                    break;
                case 2:
                    TRACE("Starting rumbler");
                    RumblerTest{}.run();
                    break;
                case 3: 
                    TRACE("Starting game");
                    SlidingPuzzle{}.run();
                    break;
            }
        }
    }
    */
    //cpu::overclock();
    //cpuOverclock(133000000, false);
    //cpuOverclock(150000000, true);

    //cpuOverclock(150000000);
    //cpuOverclock();

//    AudioTestTone test;
//    test.run();

//    start(RawAudioTest{});

//    USBMassStorage game;
//    game.run();
//    Menu{}.run();
    //AVRStatusTest{}.run();
    //start(AVRStatusTest{});

    //AVRStatusTest test;
    //test.run();
}



#ifdef HAHA



#ifdef FOO

void sdtest() {
    sd_card_t *pSD = sd_get_by_num(0);
    

    FRESULT fr = f_mount(&pSD->fatfs, pSD->pcName, 1);
    if (FR_OK != fr) panic("f_mount error: %s (%d)\n", FRESULT_str(fr), fr);
    FIL fil;
    const char* const filename = "audio/test2.txt";
    fr = f_open(&fil, filename, FA_OPEN_APPEND | FA_WRITE);
    if (FR_OK != fr && FR_EXIST != fr)
        panic("f_open(%s) error: %s (%d)\n", filename, FRESULT_str(fr), fr);
    if (f_printf(&fil, "Hello, world!\n") < 0) {
        printf("f_printf failed\n");
    }
    fr = f_close(&fil);
    if (FR_OK != fr) {
        printf("f_close error: %s (%d)\n", FRESULT_str(fr), fr);
    }
    f_unmount(pSD->pcName);

}

#endif

using DP = display_profile::RGBDouble;
using Color = DP::Color;

void rckid_main() {
    /*
    SDCardTest sdTest;
    sdTest.run();
    */
    //Serial::initialize();
    //sdtest();

    //uint16_t val = 56;

    /*
    Audio::startPlayback(SampleRate::kHz8, nullptr, 0, [&](uint16_t * x, size_t l) {
        x[0] = val;
    });
    */

    initializeIO();
    //Audio::initialize();
    //uint16_t buffer[200];
    //for (size_t i = 0; i < 200; ++i)
    //    buffer[i] = (i >= 100) ? 126 : 130;
    //Audio::startPlayback(SampleRate::kHz44_1, buffer, sizeof(buffer) / 4, [](uint16_t * x, size_t l) {});
    ST7789::initialize<DP>();
    Canvas<DP::Color> c{DP::Width, DP::Height};
    c.setFg(Color{255,255,255});
    c.setFont(Org_01);
    SD::mount();
    size_t sdBytes = SD::totalBytes() / 1000000_u64;
    uint8_t bg = 0;
    while (true) {
        // show the SD card info
        c.text(0,0);
        c.text() << "Total: " << sdBytes << "\n";

        ST7789::waitVSync();
        ST7789::update<DP>(c.rawPixels(), DP::Width, DP::Height);
        ST7789::waitUpdateDone();
        c.setBg(Color{bg, 0, 0});
        bg += 4;
        c.fill();
    }
}


int mainDisplay() {
    //cpu_overclock_max();
    //Serial::initialize();
    initializeIO();
    ST7789::initialize<DP>();
    //ST7789::initialize();
    //printf("Initialized --test\n");
    //sd::test();
    //gpio::initialize();
    //gpio::output(15);

    //ST7789::enterContinuousMode(320, 240);
    Canvas<DP::Color> c{DP::Width, DP::Height};
    c.setFg(Color{255,255,255});
    c.setFont(Org_01);
    //c.text("Hello world!", 0, 25);
    //c.pixel(0,0, Color::Blue());
    i2c_init(i2c0, 100000);
    gpio_set_function(RP_PIN_SDA, GPIO_FUNC_I2C);
    gpio_set_function(RP_PIN_SCL, GPIO_FUNC_I2C);
    // Make the I2C pins available to picotool
    bi_decl(bi_2pins_with_func(RP_PIN_SDA, RP_PIN_SCL, GPIO_FUNC_I2C));  
    uint8_t bg = 0;
    cmd::SetBrightness b{128};
    while (true) {
        /*
        if (bg < 128) {
            b.value = 0;
            i2c_write_blocking(i2c0, AVR_I2C_ADDRESS, (uint8_t*) & b, sizeof (b), false);
        } else {
            b.value = 128;
            i2c_write_blocking(i2c0, AVR_I2C_ADDRESS, (uint8_t*) & b, sizeof (b), false);
        }
        */
        State state;
        i2c_read_blocking(i2c0, AVR_I2C_ADDRESS, (uint8_t *)& state, sizeof(State), false);
        c.text(0,0);
        c.text() << (state.status.dpadLeft() ? "L " : "  ");
        c.text() << (state.status.dpadRight() ? "R " : "  ");
        c.text() << (state.status.dpadUp() ? "U " : "  ");
        c.text() << (state.status.dpadDown() ? "D " : "  ");
        c.text() << (state.status.btnA() ? "A " : "  ");
        c.text() << (state.status.btnB() ? "B " : "  ");
        c.text() << (state.status.btnSelect() ? "SEL " : "    ");
        c.text() << (state.status.btnStart() ? "START " : "      ");
        // first do I2C scan
        c.text(0, 20);
        c.text() << (state.status.dcPower() ? "DC " : "   ");
        c.text() << (state.status.charging() ? "CHRG " : "     ");
        c.text() << (state.status.headphones() ? "HP " : "   ");
        c.text() << state.info.vcc() << " " << state.info.temp();
        c.text(0, 40);
        c.text() << state.time.minutes() << ":" << state.time.seconds() << " bright:" << state.config.backlight();
        ST7789::waitVSync();
        ST7789::update<DP>(c.rawPixels(), DP::Width, DP::Height);
        //ST7789::waitVSync();
        //ST7789::updateContinuous(c.rawPixels(), c.rawPixelsCount());
        ST7789::waitUpdateDone();
        c.setBg(Color{bg, 0, 0});
        bg += 4;
        c.fill();
        /*
        ST7789::waitUpdateDone();
        ST7789::updateContinuous(c.rawPixels(), c.rawPixelsCount());
        ST7789::waitUpdateDone();
        ST7789::updateContinuous(c.rawPixels(), c.rawPixelsCount());
        ST7789::waitUpdateDone();
        ST7789::updateContinuous(c.rawPixels(), c.rawPixelsCount());
        ST7789::waitUpdateDone();
        */
        sleep_ms(50);
        //while (true);
    }
    /*
    uint8_t rxd;
    c.text(0,0) << "I2C Scan: ";
    for (uint8_t x = 0; x < 128; ++x) {
        if (i2c_read_blocking(i2c0, x, &rxd, 1, false) >= 0)
            c.text() << " " << x;
    } 
    ST7789::updateContinuous(c.rawPixels(), c.rawPixelsCount());

    while (true);
    */
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
    /*
    while (true) {
        gpio::high(15);
        cpu::delayMs(100);
        gpio::low(15);
        cpu::delayMs(100);
    } */
    return 0;
}

#endif