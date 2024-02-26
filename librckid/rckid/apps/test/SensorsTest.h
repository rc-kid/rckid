#pragma once

#include "rckid/app.h"
#include "rckid/audio.h"
#include "rckid/graphics/framebuffer.h"
#include "fonts/Iosevka_Mono6pt7b.h"

namespace rckid {

    class SensorsTest : public App<FrameBuffer<ColorRGB>> {
    public:

        SensorsTest()  = default;

    protected:

        void update() override {
        }

        void draw() override {
            Renderer & r = renderer();
            r.setFg(ColorRGB::White());
            r.setFont(Iosevka_Mono6pt7b);
            r.setBg(ColorRGB::RGB(bg_, 0, 0));
            //bg_ += 4;
            r.fill();
            r.textMultiline(0,0)
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