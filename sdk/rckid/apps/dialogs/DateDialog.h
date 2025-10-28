#pragma once

#include "../../ui/form.h"
#include "../../ui/style.h"
#include "../../ui/label.h"
#include "../../assets/fonts/OpenDyslexic64.h"
#include "../../assets/fonts/OpenDyslexic32.h"
#include "../../utils/interpolation.h"


namespace rckid {

    class DateDialog : public ui::Form<TinyDate> {
    public:
        String name() const override { return "DateDialog"; }

        DateDialog(TinyDate initialDate = timeNow().date):
            ui::Form<TinyDate>{Rect::XYWH(0, 144, 320, 96), /* raw */ true},
            d_{Rect::XYWH(0, 20, 90, 76), ""},
            m_{Rect::XYWH(110, 20, 50, 76), ""},
            y_{Rect::XYWH(180, 20, 150, 76), ""},
            dow_{Rect::XYWH(0, 8, 110, 32), ""},
            mon_{Rect::XYWH(120, 8, 200, 32), ""},
            sep1_{Rect::XYWH(90, 20, 20, 76), "/"},
            sep2_{Rect::XYWH(160, 20, 20, 76), "/"} {
            g_.addChild(d_);
            g_.addChild(m_);
            g_.addChild(y_);
            g_.addChild(dow_);
            g_.addChild(mon_);
            g_.addChild(sep1_);
            g_.addChild(sep2_);
            d_.setFont(Font::fromROM<assets::OpenDyslexic64>());
            d_.setHAlign(HAlign::Right);
            m_.setFont(Font::fromROM<assets::OpenDyslexic64>());
            m_.setHAlign(HAlign::Right);
            y_.setFont(Font::fromROM<assets::OpenDyslexic64>());
            y_.setHAlign(HAlign::Left);
            sep1_.setFont(Font::fromROM<assets::OpenDyslexic64>());
            sep2_.setFont(Font::fromROM<assets::OpenDyslexic64>());
            dow_.setFont(Font::fromROM<assets::OpenDyslexic32>());
            mon_.setFont(Font::fromROM<assets::OpenDyslexic32>());
            dow_.setHAlign(HAlign::Right);
            mon_.setHAlign(HAlign::Left);
            a_.startContinuous();
            dd_ = initialDate.day();
            mm_ = initialDate.month();
            yy_ = initialDate.year();
            updateValues();
        }

    protected:

        void update() override {
            ui::Form<TinyDate>::update();
            if (btnPressed(Btn::A)) {
                TinyDate t{dd_, mm_, yy_};
                exit(t);
            }
            if (btnPressed(Btn::B))
                exit();
            if (btnPressed(Btn::Up)) {
                switch (active_) {
                    case 0:
                        dd_ = (dd_ % TinyDate::daysInMonth(mm_, yy_)) + 1;
                        break;
                    case 1:
                        mm_ = (mm_ % 12) + 1;
                        if (dd_ > TinyDate::daysInMonth(mm_, yy_))
                            dd_ = TinyDate::daysInMonth(mm_, yy_);
                        break;
                    case 2:
                        yy_ = (yy_ + 1) % 4096;
                        if (dd_ > TinyDate::daysInMonth(mm_, yy_))
                            dd_ = TinyDate::daysInMonth(mm_, yy_);
                        break;
                }
                updateValues();
            }
            if (btnPressed(Btn::Down)) {
                switch (active_) {
                    case 0:
                        dd_ = (dd_ + TinyDate::daysInMonth(mm_, yy_) - 2) % TinyDate::daysInMonth(mm_, yy_) + 1;
                        break;
                    case 1:
                        mm_ = (mm_ + 10) % 12 + 1;
                        if (dd_ > TinyDate::daysInMonth(mm_, yy_))
                            dd_ = TinyDate::daysInMonth(mm_, yy_);
                        break;
                    case 2:
                        yy_ = (yy_ + 4095) % 4096;
                        if (dd_ > TinyDate::daysInMonth(mm_, yy_))
                            dd_ = TinyDate::daysInMonth(mm_, yy_);
                        break;
                }
                updateValues();
            }
            if (btnPressed(Btn::Left) || btnPressed(Btn::Right)) {
                switch (active_) {
                    case 0:
                        d_.setColor(ui::Style::fg());
                        break;
                    case 1:
                        m_.setColor(ui::Style::fg());
                        break;
                    case 2:
                        y_.setColor(ui::Style::fg());
                        break;
                }
                if (btnPressed(Btn::Left))
                    active_ = (active_ == 0) ? 2 : active_ - 1;
                else
                    active_ = (active_ + 1) % 3;
            }
            a_.update();
        }

        void draw() override {
            switch (active_) {
                case 0:
                    d_.setColor(ui::Style::accentFg().withAlpha(interpolation::cosineLoop(a_, 0, 255).round()));
                    break;
                case 1:
                    m_.setColor(ui::Style::accentFg().withAlpha(interpolation::cosineLoop(a_, 0, 255).round()));
                    break;
                case 2:
                    y_.setColor(ui::Style::accentFg().withAlpha(interpolation::cosineLoop(a_, 0, 255).round()));
                    break;
            }
            ui::Form<TinyDate>::draw();
        }

        void updateValues() {
            d_.setText(STR(fillLeft(dd_, 2, ' ')));
            m_.setText(STR(fillLeft(mm_, 2, ' ')));
            y_.setText(STR(fillLeft(yy_, 2, ' ')));
            dow_.setText(STR(TinyDate::dayOfWeek(dd_, mm_, yy_)));
            mon_.setText(STR(static_cast<TinyDate::Month>(mm_)));
        }

    private:

        Timer a_{1000};

        ui::Label d_;
        ui::Label m_;
        ui::Label y_;
        ui::Label dow_;
        ui::Label mon_;
        ui::Label sep1_;
        ui::Label sep2_;
        uint8_t mm_ = 0;
        uint8_t dd_ = 0;
        uint16_t yy_ = 0;
        uint8_t active_ = 0;

    }; // rckid::TimeDialog

} // namespace rckid