#pragma once

#include "../../app.h"
#include "../../ui/form.h"
#include "../../ui/label.h"
#include "../../assets/fonts/OpenDyslexic128.h"
#include "../../ui/tilemap.h"



namespace rckid {


    extern uint32_t sd_write_blocks;

    class SDTest : public ui::Form<void> {
    public:
        String name() const override { return "SDTest"; }

        SDTest():
            ui::Form<void>{},
            text_{40,15, assets::System16, palette_} {
            g_.addChild(text_);
            text_.setPos(0, 16);

        }

        void update() override {
            ui::Form<void>::update();
            switch (test_) {
                case NO_TEST: {
                    if (btnPressed(Btn::A)) {
                        test_ = TEST_1k;
                        text_.text(0, 2) << "1k";
                    }
                    break;
                }
                case TEST_1k: {
                    test_ = TEST_10k;
                    uint32_t t = test(1024);
                    text_.text(15, 2) << t;
                    text_.text(30, 2) << after;
                    text_.text(0, 3) << "10k";
                    break;
                }
                case TEST_10k: {
                    test_ = TEST_100k;
                    uint32_t t = test(10240);
                    text_.text(15, 3) << t;
                    text_.text(30, 3) << after;
                    text_.text(0, 4) << "100k";
                    break;
                }
                case TEST_100k: {
                    test_ = TEST_1M;
                    uint32_t t = test(102400);
                    text_.text(15, 4) << t;
                    text_.text(30, 4) << after;
                    text_.text(0, 5) << "1M";
                    break;
                }
                case TEST_1M: {
                    test_ = NO_TEST;
                    uint32_t t = test(1024000);
                    text_.text(15, 5) << t;
                    text_.text(30, 5) << after;
                    break;
                }
            }
        }


    private:
// total    block   busy
// 33828    1036    19324
// 703960   4746    671018
// 4278289  41849   4149817
// 43360066 412867  42100412
        uint32_t test(uint32_t numBytes) {
            uint32_t before = sd_write_blocks;
            uint64_t startTime = uptimeUs64();
            fs::FileWrite f{fs::fileWrite("/sdtest.bin")};
            if (! f.good()) {
                text_.text(0, 14) << "Cannot open file for write";
                return 0;
            }
            for (uint32_t i = 0; i < numBytes; ++i) {
                uint8_t x = i & 0xff;
                f.write(&x, 1);
            }
            f.close();
            uint64_t endTime = uptimeUs64();
            uint32_t durationUs = endTime - startTime;
            uint32_t speedKbps = (numBytes / 1024.0f) / (durationUs / 1000000.0f);
            after = sd_write_blocks - before;
            //return speedKbps;
            return durationUs;
        }

        static constexpr uint8_t NO_TEST = 0;
        static constexpr uint8_t TEST_1k = 1;
        static constexpr uint8_t TEST_10k = 2;
        static constexpr uint8_t TEST_100k = 3;
        static constexpr uint8_t TEST_1M = 4;

        uint8_t test_ = NO_TEST;

        uint32_t after = 0;

        ui::Tilemap<Tile<8, 16, Color16>> text_;


        static constexpr uint16_t palette_[] = {
            // gray
            ColorRGB{0x00, 0x00, 0x00}.toRaw(), 
            ColorRGB{0x11, 0x11, 0x11}.toRaw(), 
            ColorRGB{0x22, 0x22, 0x22}.toRaw(), 
            ColorRGB{0x33, 0x33, 0x33}.toRaw(), 
            ColorRGB{0x44, 0x44, 0x44}.toRaw(), 
            ColorRGB{0x55, 0x55, 0x55}.toRaw(), 
            ColorRGB{0x66, 0x66, 0x66}.toRaw(), 
            ColorRGB{0x77, 0x77, 0x77}.toRaw(), 
            ColorRGB{0x88, 0x88, 0x88}.toRaw(), 
            ColorRGB{0x99, 0x99, 0x99}.toRaw(), 
            ColorRGB{0xaa, 0xaa, 0xaa}.toRaw(), 
            ColorRGB{0xbb, 0xbb, 0xbb}.toRaw(), 
            ColorRGB{0xcc, 0xcc, 0xcc}.toRaw(), 
            ColorRGB{0xdd, 0xdd, 0xdd}.toRaw(), 
            ColorRGB{0xee, 0xee, 0xee}.toRaw(), 
            ColorRGB{0xff, 0xff, 0xff}.toRaw(), 
            0, 
            ColorRGB{0xff, 0xff, 0xff}.toRaw(), 
            ColorRGB{0x00, 0xff, 0x00}.toRaw(),
        };

    

    }; // rckid::SDTest
} // namespace rckid