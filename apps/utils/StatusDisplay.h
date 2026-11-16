#pragma once

#include <kiddo/app.h>
#include <kiddo/graphics/canvas.h>

#include <kiddo/ui/header.h>

namespace kiddo {

    class StatusDisplay : public GraphicsApp<Canvas<ColorRGB>> {
    public:
        static void run() {
            StatusDisplay app{};
            app.loop();
        }

    protected:

        StatusDisplay(): GraphicsApp{ARENA(Canvas<ColorRGB>{320, 240})} {}

        void update() {
            GraphicsApp::update();
        }

        void draw() override {
            g_.fill();

            g_.text(0, 20) <<
                (kiddo::dcPower() ? "DC " : "   ") <<
                (kiddo::charging() ? "CHRG " : "     ") << 
                (kiddo::audioEnabled() ? "AUD " : "    ") << 
                (kiddo::audioHeadphones() ? "HP " : "   ") <<
                (kiddo::btnDown(Btn::Home) ? "Home " : "     ") << 
                (kiddo::btnDown(Btn::VolumeUp) ? "V+ " : "   ") << 
                (kiddo::btnDown(Btn::VolumeDown) ? "V- " : "   ") <<
                (kiddo::btnDown(Btn::Left) ? "L " : "  ") << 
                (kiddo::btnDown(Btn::Right) ? "R " : "  ") << 
                (kiddo::btnDown(Btn::Up) ? "U " : "  ") << 
                (kiddo::btnDown(Btn::Down) ? "D " : "  ") << 
                (kiddo::btnDown(Btn::A) ? "A " : "  ") << 
                (kiddo::btnDown(Btn::B) ? "B " : "  ") << 
                (kiddo::btnDown(Btn::Select) ? "Sel " : "    ") << 
                (kiddo::btnDown(Btn::Start) ? "Start " : "      ");
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
                "\nDebug mode   " << (debugMode() ? "yes" : "no") << 
                "\nCart FS size " << cartridgeCapacity() <<
                "\nSD  size     " << sdCapacity();

            Header::drawOn(g_);
        }
        

    }; // kiddo::StatusDisplay


} // namespace kiddo
