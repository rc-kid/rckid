#define HAHA_
#ifdef HAHA

#include "rckid/app.h"
#include "rckid/stats.h"
#include "rckid/graphics/framebuffer.h"
#include "rckid/graphics/font.h"
#include "assets/fonts/Iosevka_16.h"


#include "assets/all.h"

#include "rckid/audio/tone.h"
#include "rckid/audio/music.h"
#include "rckid/sd.h"

using namespace rckid;

constexpr NoteInfo Octave[] = {
    NOTE_4(C4), 
    NOTE_4(D4),
    NOTE_4(E4),
    NOTE_4(F4), 
    NOTE_4(G4),
    NOTE_4(A4),
    NOTE_4(B4),
    NOTE_4(C5),
    REST_4,
    NOTE_4(C5),
    NOTE_4(B4),
    NOTE_4(A4),
    NOTE_4(G4),
    NOTE_4(F4),
    NOTE_4(E4),
    NOTE_4(D4),
    NOTE_4(C4),
    REST_4,
};

class SimpleApp : public App<FrameBuffer<ColorRGB>> {
public:
    uint8_t buffer[2048];

    SimpleApp() {
        SD::initialize();
        for (int i = 0; i < 512; ++i)
            buffer[i] = i & 0xff;
        SD::writeBlock(1234, buffer);
        SD::readBlock(1234, buffer);
        /*
        // to initialize, the SPI baudrate must be between 100-400kHz for the initialization
        spi_init(RP_SD_SPI, 200000);
        gpio_set_function(RP_PIN_SD_SCK, GPIO_FUNC_SPI);
        gpio_set_function(RP_PIN_SD_TX, GPIO_FUNC_SPI);
        gpio_set_function(RP_PIN_SD_RX, GPIO_FUNC_SPI);
        bi_decl(bi_3pins_with_func(RP_PIN_SD_SCK, RP_PIN_SD_TX, RP_PIN_SD_RX, GPIO_FUNC_SPI));
        // initialize the CS for manual control
        gpio::outputHigh(RP_PIN_SD_CSN);
        // create the buffer and fill it with 0xff
        uint8_t buffer[16];
        memset(buffer, 0xff, sizeof(buffer));
        // while CS is high, send at least 74 times 0xff
        spi_write_blocking(RP_SD_SPI, buffer, sizeof(buffer));
        // now send the CMD_RESET command
        cmd0Status_ = SD::sendCommand({0x40, 0x00, 0x00, 0x00, 0x00, 0x95});
        // try sending the Interface condition
        cmd8Status_ = SD::sendCommand({0x48, 0x00, 0x00, 0x01, 0xaa, 0x87}, buffer, 4);        
        cmd8TestVal_ = buffer[3];
        */
    }

protected:

    void update() override {
        /*
        errors = 0;
        MEASURE_TIME(naive_, {
            for (int i = 0; i < 240; ++i)
                b16[i] = palette[b8[i]];
        });
        MEASURE_TIME(cpp_, {
            ColorRGB_332::translatePixelBuffer((uint32_t*)b8, (uint32_t*)b16, 240);
        });
        MEASURE_TIME(asm_, {
            uint8_t const * res = rckid_color256_to_rgb(b8, b16, 240, palette);
            errors = b16[0];
        });
        */
    }

    void draw() override {
        driver_.fill();
        auto w = driver_.textMultiline(0, 0) << "size: " << SD::capacity() << "\n";
        for (int i = 0; i < 512; ++i) {
            //if (buffer[i] != 255) {
                w << buffer[i];
                w << ",";
            //}
        }
    }

    //SD::Status cmd0Status_;
    //SD::Status cmd8Status_;
    uint8_t cmd8TestVal_;


}; // SimpleApp


int main() {
    rckid::initialize();
    ST7789::setFPS(FPS::FPS_40);

    //Pong{}.run();
    SimpleApp{}.run();

    cpu::overclock();
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

#endif


