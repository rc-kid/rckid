#pragma once

#include "../../ui/form.h"
#include "../../ui/style.h"
#include "../../ui/label.h"
#include "../../assets/fonts/OpenDyslexic128.h"
#include "../../utils/interpolation.h"


namespace rckid {

    class TimeDialog : public ui::Form<TinyTime> {
    public:
        String name() const override { return "TimeDialog"; }

        /** Dialog budgeting mirrors that of its parent.
         */
        bool isBudgeted() const override { 
            if (parent() != nullptr) 
                return parent()->isBudgeted();
            return true;
        }

        TimeDialog(TinyTime initialTime = timeNow().time):
            ui::Form<TinyTime>{Rect::XYWH(0, 144, 320, 96), /* raw */ true},
            h_{Rect::XYWH(0, 0, 150, 96), ""},
            m_{Rect::XYWH(170, 0, 150, 96), ""},
            colon_{Rect::XYWH(150, 0, 20, 96), ":"} {
            g_.addChild(h_);
            g_.addChild(m_);
            g_.addChild(colon_);
            h_.setFont(Font::fromROM<assets::OpenDyslexic128>());
            h_.setHAlign(HAlign::Right);
            m_.setFont(Font::fromROM<assets::OpenDyslexic128>());
            m_.setHAlign(HAlign::Left);
            colon_.setFont(Font::fromROM<assets::OpenDyslexic128>());
            a_.startContinuous();
            hh_ = initialTime.hour();
            mm_ = initialTime.minute();
            updateValues();
        }

    protected:

        void update() override {
            ui::Form<TinyTime>::update();
            if (btnPressed(Btn::A)) {
                TinyTime t{hh_, mm_};
                exit(t);
            }
            if (btnPressed(Btn::B))
                exit();
            if (btnPressed(Btn::Up)) {
                if (activeH_)
                    hh_ = (hh_ + 1) % 24;
                else
                    mm_ = (mm_ + 1) % 60;
                updateValues();
            }
            if (btnPressed(Btn::Down)) {
                if (activeH_)
                    hh_ = (hh_ + 23) % 24;
                else
                    mm_ = (mm_ + 59) % 60;
                updateValues();
            }
            if (btnPressed(Btn::Left) || btnPressed(Btn::Right)) {
                activeH_ = !activeH_;
                if (activeH_)
                    m_.setColor(ui::Style::fg());
                else
                    h_.setColor(ui::Style::fg());
            }
            a_.update();
        }

        void draw() override {
            if (activeH_) 
                h_.setColor(ui::Style::accentFg().withAlpha(interpolation::cosineLoop(a_, 0, 255).round()));
            else
                m_.setColor(ui::Style::accentFg().withAlpha(interpolation::cosineLoop(a_, 0, 255).round()));
            ui::Form<TinyTime>::draw();
        }

        void updateValues() {
            h_.setText(STR(fillLeft(hh_, 2, '0')));
            m_.setText(STR(fillLeft(mm_, 2, '0')));
        }

    private:

        Timer a_{1000};

        ui::Label h_;
        ui::Label m_;
        ui::Label colon_;
        uint8_t hh_ = 0;
        uint8_t mm_ = 0;
        bool activeH_ = true;

    }; // rckid::TimeDialog

} // namespace rckid