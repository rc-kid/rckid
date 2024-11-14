#pragma once

#include <rckid/app.h>
#include <rckid/graphics/canvas.h>
#include <rckid/assets/fonts/OpenDyslexic128.h>
#include <rckid/assets/fonts/OpenDyslexic64.h>
#include <rckid/ui/timer.h>
#include <rckid/utils/interpolation.h>


#include <rckid/ui/header.h>


namespace rckid {

    /** Allows setting the current date and time. 

        
     */
    class SetTime : public GraphicsApp<Canvas<ColorRGB>> {
    public:

        static void run() {
            SetTime app{};
            app.loop();
        }

    protected:

        SetTime(): GraphicsApp{ARENA(Canvas<ColorRGB>{320, 240})} {
            d_ = dateTime();
            t_.startContinuous();
        }

        void update() override {
            GraphicsApp::update();
            if (btnPressed(Btn::Left)) {
                if (ae_ == AE_HOUR)
                    ae_ = AE_YEAR;
                else
                    --ae_;
            }
            if (btnPressed(Btn::Right)) {
                if (ae_ == AE_YEAR)
                    ae_ = AE_HOUR;
                else 
                    ++ae_;
            }
            if (btnPressed(Btn::Up))
                adjustSelectedBy(1);
            if (btnPressed(Btn::Down))
                adjustSelectedBy(-1);
            // sets the date & time if button A is pressed
            if (btnPressed(Btn::A)) {
                setDateTime(d_);
                exit();
            }
        }

        /** Draws the date & time */
        void draw() override {
            t_.update();
            int x = interpolation::easingCos(t_, 0, 511);
            Color active = Color{x < 256 ? x : 256 - (x - 255), 0, 0};
            g_.fill();
            TinyDate now = dateTime();
            Font const & f = assets::font::OpenDyslexic128::font;
            Font const & fSmall = assets::font::OpenDyslexic64::font;

            std::string h{STR(fillLeft(d_.hour(), 2, '0'))};
            std::string m{STR(fillLeft(d_.minute(), 2, '0'))};
            int hWidth = f.textWidth(h.c_str());
            g_.text(150 - hWidth, 30, f, (ae_ == AE_HOUR) ? active : color::White) << h;
            g_.text(170, 30, f,  (ae_ == AE_MINUTE) ? active : color::White) << m;
            if (now.second() & 1)
                g_.text(160 - f.glyphInfoFor(':').advanceX / 2, 30, f, color::White) << ':';

            std::string day{STR(fillLeft(d_.day(), 2, ' '))};
            std::string month{STR(fillLeft(d_.month(), 2, ' '))};
            std::string year{STR(fillLeft(d_.year(), 4, ' '))};

            g_.text(25, 130, fSmall,  (ae_ == AE_DAY) ? active : color::White) << day;
            g_.text(114, 130, fSmall,  (ae_ == AE_MONTH) ? active : color::White) << month;
            g_.text(203, 130, fSmall,  (ae_ == AE_YEAR) ? active : color::White) << year;
            g_.text(82, 130, fSmall, color::White) << "/";
            g_.text(171, 130, fSmall, color::White) << "/";

            // TODO draw alarm clock as well and allow its setting
            Header::drawOn(g_);
        }

        void adjustSelectedBy(int by) {
            switch (ae_) {
                case AE_HOUR:
                    d_.setHour(d_.hour() + by);
                    break;
                case AE_MINUTE:
                    d_.setMinute(d_.minute() + by);
                    break;
                case AE_DAY:
                    d_.setDay(d_.day() + by);
                    break;
                case AE_MONTH:
                    d_.setMonth(d_.month() + by);
                    break;
                case AE_YEAR:
                    d_.setYear(d_.year() + by);
                    break;
                default:
                    UNREACHABLE;
            }
        }



        TinyDate d_;
        Timer t_{500};

        static constexpr unsigned AE_HOUR = 0;
        static constexpr unsigned AE_MINUTE = 1;
        static constexpr unsigned AE_DAY = 2;
        static constexpr unsigned AE_MONTH = 3;
        static constexpr unsigned AE_YEAR = 4;
        unsigned ae_ = AE_HOUR;



    }; // rckid::SetTime


} // namespace rckid