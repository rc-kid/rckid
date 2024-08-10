#pragma once

#include "rckid/app.h"
#include "rckid/stats.h"
#include "rckid/graphics/framebuffer.h"
#include "rckid/ui/header.h"

namespace rckid {

    /** Simple application that displays the device's HW status for troubleshooting. 
     
        
     */
    class HWTest : public App<FrameBuffer<ColorRGB>> {
    public:

        static HWTest * create() { return new HWTest{}; }


    protected:

        void update() override {
            if (down(Btn::A) && down(Btn::B))
                exit();
        }

        void draw() override {
            driver_.fill(); 
            header_.drawOn(driver_, Rect::WH(320, 20));
            driver_.setFg(Color::White());
            driver_.textMultiline(10, 20) <<
                //"VCC:   " << vcc() << "\n" <<
                "VBatt: " << vBatt() << "\n" <<
                //"I:     " << current() << "\n" <<
                //"Temp:  " << tempAvr() << "\n" <<
                "als: " << lightAmbient() << "\n" <<
                "uv:  " << lightUV();
            driver_.textMultiline(100, 20) << 
                "aX:  " << accelX() << "\n" << 
                "aY:  " << accelY() << "\n" <<
                "aZ:  " << accelZ() << "\n" <<
                "gX:  " << gyroX() << "\n" << 
                "gY:  " << gyroY() << "\n" <<
                "gZ:  " << gyroZ();
            // now do the accelerometer dot
            drawAccelerometer(240, 100, 80);
            //driver_.setPixelAt(160 + 80, 20 + 80, Color::Gray());
            //driver_.setFg(Color::Red());
        }

        void drawAccelerometer(int x, int y, int radius) {
            driver_.setFg(Color::Gray());
            driver_.drawRect(Rect::XYWH(x - 1, y -1, 2, 2));
            // draw gyroscope (yellow)
            int ax = gyroX();
            int ay = gyroY();
            ax = ax * radius / 32768;
            ay = ay * radius / 32768;
            driver_.setFg(Color::Yellow());
            driver_.drawRect(Rect::XYWH(x + ax - 1, y + ay -1, 2, 2));

            // draw accelerometer (red)
            ax = accelX();
            ay = accelY();
            ax = ax * radius / 32768;
            ay = ay * radius / 32768;
            driver_.setFg(Color::Red());
            driver_.drawRect(Rect::XYWH(x + ax - 1, y + ay -1, 2, 2));
        }


    private:

        header::Renderer<Color> header_;

    }; 

}
