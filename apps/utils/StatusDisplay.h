#pragma once

#include <rckid/app.h>
#include <rckid/graphics/canvas.h>

#include <rckid/ui/header.h>

namespace rckid {

    class StatusDisplay : public GraphicsApp<Canvas<ColorRGB>> {
    public:
        static void run() {
            StatusDisplay app{};
            app.loop();
        }

    protected:

        StatusDisplay(): GraphicsApp{ARENA(Canvas<ColorRGB>{320, 240})} {}

        void draw() override {
            g_.fill();

            g_.text(0, 20) <<
                (rckid::dcPower() ? "DC " : "   ") <<
                (rckid::charging() ? "CHRG " : "     ") << 
                (rckid::audioEnabled() ? "AUD " : "    ") << 
                (rckid::audioHeadphones() ? "HP " : "   ") <<
                (rckid::btnDown(Btn::Home) ? "Home " : "     ") << 
                (rckid::btnDown(Btn::VolumeUp) ? "V+ " : "   ") << 
                (rckid::btnDown(Btn::VolumeDown) ? "V- " : "   ") <<
                (rckid::btnDown(Btn::Left) ? "L " : "  ") << 
                (rckid::btnDown(Btn::Right) ? "R " : "  ") << 
                (rckid::btnDown(Btn::Up) ? "U " : "  ") << 
                (rckid::btnDown(Btn::Down) ? "D " : "  ") << 
                (rckid::btnDown(Btn::A) ? "A " : "  ") << 
                (rckid::btnDown(Btn::B) ? "B " : "  ") << 
                (rckid::btnDown(Btn::Select) ? "Sel " : "    ") << 
                (rckid::btnDown(Btn::Start) ? "Start " : "      ");
            g_.text(0, 40) << 
                "\nVBatt:   " << vBatt() << 
                "\nTempAvr: " << tempAvr() << 
                "\nAccelX:  " << accelX() << 
                "\nAccelY:  " << accelY() << 
                "\nAccelZ:  " << accelZ() << 
                "\nGyroX:   " << gyroX() << 
                "\nGyroY:   " << gyroY() << 
                "\nGyroZ:   " << gyroZ() << 
                "\nLight:   " << lightAmbient() << 
                "\nUV:      " << lightUV(); 
            g_.text(120, 40) <<
                "\nCart FS size " << cartridgeCapacity() <<
                "\nSD  size     " << sdCapacity();

            Header::drawOn(g_);
        }
        

    }; // rckid::StatusDisplay


} // namespace rckid
