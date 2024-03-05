#pragma once

#include "rckid/app.h"
#include "rckid/audio.h"
#include "rckid/graphics/framebuffer.h"
#include "fonts/Iosevka_Mono6pt7b.h"

namespace rckid {

    class SensorsTest : public FBApp<FrameBuffer<ColorRGB>> {
    public:

        SensorsTest()  = default;

    protected:

        void update() override {
        }

        void draw() override {
            fb_.setFg(ColorRGB::White());
            fb_.setFont(Iosevka_Mono6pt7b);
            fb_.setBg(ColorRGB::RGB(bg_, 0, 0));
            //bg_ += 4;
            fb_.fill();
            fb_.textMultiline(0,0)
                << "accX:  " << accelX() << "\n"
                << "accY:  " << accelY() << "\n"
                << "accZ:  " << accelZ() << "\n"
                << "gyroX: " << gyroX() << "\n"
                << "gyroY: " << gyroY() << "\n"
                << "gyroZ: " << gyroZ() << "\n\n"
                << "als:  " << lightAmbient() << '\n'
                << "uv:   " << lightUV();
       }

    private:
        uint8_t bg_ = 0;

    }; 
}