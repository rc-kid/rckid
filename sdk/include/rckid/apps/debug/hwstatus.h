#pragma once

#include <rckid/ui/app.h>
#include <rckid/ui/label.h>
#include <rckid/ui/image.h>
#include <assets/Iosevka24.h>

namespace rckid {

    class HWStatus : public ui::App<void> {
    public:
        String name() const override { return "HWStatus"; }

        HWStatus() {
            using namespace ui;
            text_ = addChild(new ui::TileGrid{40, 15, Palette256})
                << SetRect(Rect::WH(320, 240));
            tg_ = & text_->contents();
              
            tg_->text(0, 1) << "Buttons:";
            tg_->text(0, 2) << "Accel:";
            tg_->text(0, 5) << "Steps:";
            tg_->text(0, 6) << "Audio:";
            tg_->text(0, 7) << "Time:";
            tg_->text(0, 8) << "Budget:";
            tg_->text(0, 9) << "Power:";

        }

    protected:
    
        void loop() override {
            ui::App<void>::loop();
            tg_->text(15, 1) << (btnDown(Btn::Up) ? "U " : "  ");
            tg_->text(17, 1) << (btnDown(Btn::Down) ? "D " : "  ");
            tg_->text(19, 1) << (btnDown(Btn::Left) ? "L " : "  ");
            tg_->text(21, 1) << (btnDown(Btn::Right) ? "R " : "  ");
            tg_->text(23, 1) << (btnDown(Btn::A) ? "A " : "  ");
            tg_->text(25, 1) << (btnDown(Btn::B) ? "B " : "  ");
            tg_->text(27, 1) << (btnDown(Btn::Select) ? "Sel " : "    ");
            tg_->text(31, 1) << (btnDown(Btn::Start) ? "Start " : "      ");

            Point3D acc = hal::io::accelerometerState();
            tg_->text(15, 2) << "X: " << acc.x << "    ";
            tg_->text(15, 3) << "Y: " << acc.y << "    ";
            tg_->text(15, 4) << "Z: " << acc.z << "    ";

            /*
            tg_->text(15, 5) << pedometerCount() << "    ";
            */
            tg_->text(15, 6) << (audio::isPlaying() ? "P " : "  ");
            tg_->text(17, 6) << (audio::isRecording() ? "R " : "  ");
            tg_->text(19, 6) << (audio::headphonesConnected() ? "HP " : "   ");
            tg_->text(22, 6) << audio::volume() << "  ";

            tg_->text(15, 5) << time::now() << "      ";
            tg_->text(25, 7) << time::uptimeUs() / 1000000;

            // tg_->text(15, 8) << (isBudgeted() ? "Y " : "N  ");
            tg_->text(17, 8) << pim::remainingBudget();

            tg_->text(15, 9) << (power::dcConnected() ? "USB " : "    ");
            tg_->text(19, 9) << (power::charging() ? "CHRG " : "     ");
            tg_->text(24, 9) << power::vcc();

        }
    

    private:

        ui::TileGrid * text_;
        TileGrid * tg_;
        

    }; // rckid::HWStatus

} // namespace rckid