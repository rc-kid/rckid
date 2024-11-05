#pragma once

#include <rckid/app.h>
#include <rckid/graphics/canvas.h>
#include <rckid/assets/fonts/OpenDyslexic128.h>
#include <rckid/assets/fonts/OpenDyslexic64.h>
#include <rckid/assets/fonts/Iosevka32.h>
#include <rckid/assets/fonts/Symbols64.h>


#include <rckid/ui/header.h>

namespace rckid {

    class Clock : public GraphicsApp<Canvas<ColorRGB>> {
    public:
        static void run() {
            Clock app{};
            app.loop();
        }

    protected:

        Clock(): GraphicsApp{Canvas<ColorRGB>{320, 240}} {
            alarm_ = alarm();
            t_.startContinuous();
        }

        void update() override {
            GraphicsApp::update();
            if (ae_ != 0) {
                if (btnPressed(Btn::Left)) {
                    if (ae_ == AE_HOUR)
                        ae_ = AE_SUN;
                    else
                        --ae_;
                }
                if (btnPressed(Btn::Right)) {
                    if (ae_ == AE_SUN)
                        ae_ = AE_HOUR;
                    else 
                        ++ae_;
                }
                if (btnPressed(Btn::Up))
                    adjustSelectedBy(1);
                if (btnPressed(Btn::Down))
                    adjustSelectedBy(-1);
                if (btnPressed(Btn::A)) {
                    ae_ = 0;
                    setAlarm(alarm_);
                }
            } else if (btnPressed(Btn::Select)) {
                ae_ = AE_HOUR;
            }
        }

        void draw() override {
            t_.update();
            int x = interpolation::easingCos(t_, 0, 511);
            Color active = Color{x < 256 ? x : 256 - (x - 255), 0, 0};

            g_.fill();
            Font const & f = assets::font::OpenDyslexic128::font;
            Font const & fSmall = assets::font::OpenDyslexic64::font;
            Font const & fTiny = assets::font::Iosevka32::font;
            Font const & fSym = assets::font::Symbols64::font;

            TinyDate now = dateTime();
            std::string h{STR(fillLeft(now.hour(), 2, '0'))};
            std::string m{STR(fillLeft(now.minute(), 2, '0'))};
            int hWidth = f.textWidth(h.c_str());
            g_.text(150 - hWidth, 30, f, color::White) << h;
            g_.text(170, 30, f, color::White) << m;
            if (now.second() & 1)
                g_.text(160 - f.glyphInfoFor(':').advanceX / 2, 30, f, color::White) << ':';


            g_.text(0, 110, fSym, alarm_.enabled() ? color::White : color::DarkGray) << assets::glyph::Alarm;
            g_.text(87, 130, fSmall, ae_ == AE_HOUR ? active : color::White) << fillLeft(alarm_.hour(), 2, '0');
            g_.text(135, 130, fSmall, color::White) << ":";
            g_.text(145, 130, fSmall, ae_ == AE_MINUTE ? active : color::White) << fillLeft(alarm_.minute(), 2, '0');

            g_.text(90, 180, fTiny, ae_ == AE_MON ? active : color::White) <<  (alarm_.enabledDay(0) ? "Mo" : "-");
            g_.text(120, 180, fTiny, ae_ == AE_TUE ? active : color::White) <<  (alarm_.enabledDay(1) ? "Tu" : "-");
            g_.text(150, 180, fTiny, ae_ == AE_WED ? active : color::White) <<  (alarm_.enabledDay(2) ? "We" : "-");
            g_.text(180, 180, fTiny, ae_ == AE_THU ? active : color::White) <<  (alarm_.enabledDay(3) ? "Th" : "-");
            g_.text(210, 180, fTiny, ae_ == AE_FRI ? active : color::White) <<  (alarm_.enabledDay(4) ? "Fr" : "-");
            g_.text(240, 180, fTiny, ae_ == AE_SAT ? active : color::White) <<  (alarm_.enabledDay(5) ? "Sa" : "-");
            g_.text(270, 180, fTiny, ae_ == AE_SUN ? active : color::White) <<  (alarm_.enabledDay(6) ? "Su" : "-");

//            g_.text(3,  )


//            LOG(fSym.glyphInfoFor(assets::glyph::Alarm).advanceX); // 83
//            LOG(fSmall.textWidth("HH : MM")); // 150
            //LOG(fTiny.textWidth("MoTuWeThFrSaSu")); // 314 - 254 26, 10


            // TODO draw alarm clock as well and allow its setting
            Header::drawOn(g_);
        }

        void adjustSelectedBy(int by) {
            switch (ae_) {
                case AE_HOUR:
                    alarm_.setHour(alarm_.hour() + by);
                    break;
                case AE_MINUTE:
                    alarm_.setMinute(alarm_.minute() + by);
                    break;
                case AE_MON:
                case AE_TUE:
                case AE_WED:
                case AE_THU:
                case AE_FRI:
                case AE_SAT:
                case AE_SUN: {
                    uint8_t idx = ae_ - AE_MON;   
                    alarm_.setEnabledDay(idx, ! alarm_.enabledDay(idx));
                    break;
                }
                default:
                    UNREACHABLE;
            }

        }

        Timer t_{500};
        TinyAlarm alarm_;


        static constexpr unsigned AE_HOUR = 1;
        static constexpr unsigned AE_MINUTE = 2;
        static constexpr unsigned AE_MON = 3;
        static constexpr unsigned AE_TUE = 4;
        static constexpr unsigned AE_WED = 5;
        static constexpr unsigned AE_THU = 6;
        static constexpr unsigned AE_FRI = 7;
        static constexpr unsigned AE_SAT = 8;
        static constexpr unsigned AE_SUN = 9;
        unsigned ae_ = 0;
    }; // rckid::Clock
} // namespace rckid
