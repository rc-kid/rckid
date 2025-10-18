#pragma once

#include "../../app.h"
#include "../../ui/form.h"
#include "../../ui/label.h"
#include "../../assets/fonts/OpenDyslexic128.h"
#include "../../ui/tilemap.h"


namespace rckid {

    /** Serial monitor
     
        A simple serial monitor, that can be used to either capture serial information via cartridge pins. 
     */
    class HardwareStatus  : public ui::Form<void> {
    public:

        String name() const override { return "HardwareStatus"; }

        HardwareStatus():
            ui::Form<void>{},
            text_{40,15, assets::System16, palette_} {
            g_.addChild(text_);
            text_.setPos(0, 16);
            text_.text(0,0) << "Hardware Status";
            text_.text(0, 2) << "Buttons:";
            text_.text(0, 3) << "Accel:";
            text_.text(0, 4) << "Audio:";
            text_.text(0, 5) << "Time:";
            text_.text(0, 6) << "Budget:";
            text_.text(0, 7) << "Power:";
        }

        ~HardwareStatus() override {
        }

        void update() override {
            ui::Form<void>::update();
            text_.text(15, 2) << (btnDown(Btn::Up) ? "U " : "  ");
            text_.text(17, 2) << (btnDown(Btn::Down) ? "D " : "  ");
            text_.text(19, 2) << (btnDown(Btn::Left) ? "L " : "  ");
            text_.text(21, 2) << (btnDown(Btn::Right) ? "R " : "  ");
            text_.text(23, 2) << (btnDown(Btn::A) ? "A " : "  ");
            text_.text(25, 2) << (btnDown(Btn::B) ? "B " : "  ");
            text_.text(27, 2) << (btnDown(Btn::Select) ? "Sel " : "    ");
            text_.text(31, 2) << (btnDown(Btn::Start) ? "Start " : "      ");

            text_.text(15, 4) << (audioPlayback() ? "P " : "  ");
            text_.text(17, 4) << (audioRecording() ? "R " : "  ");
            text_.text(19, 4) << (audioHeadphones() ? "HP " : "   ");
            text_.text(22, 4) << audioVolume() << "  ";

            //text_.text(15, 5) << timeNow() << "      ";
            text_.text(25, 5) << uptimeUs64() / 1000000;

            text_.text(15, 6) << (isBudgeted() ? "Y " : "N  ");
            text_.text(17, 6) << budget();
        }

    private:
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


    }; // rckid::SerialMonitor

} // namespace rckid