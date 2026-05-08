#pragma once

#include <rckid/ui/app.h>
#include <rckid/ui/image.h>
#include <rckid/ui/label.h>

#include <assets/icons_64.h>
#include <assets/OpenDyslexic64.h>

namespace rckid {

    /** Simple dialog for selecting time (hours and minutes only).
     */
    class TimeDialog: public ui::App<TinyTime> {
    public:

        String name() const override { return "Select time"; }

        TimeDialog(TinyTime time = TinyTime{0, 0}):
            ui::App<TinyTime>{Rect::XYWH(0, 140, 320, 100)},
            hh_{time.hour()},
            mm_{time.minute()} 
        {
            using namespace ui;

            icon_ = addChild(new ui::Image{})
                << SetRect(Rect::XYWH(0, 0, 100, 100))
                << SetBitmap(assets::icons_64::alarm_clock);

            h_ = addChild(new ui::Label{})
                << SetRect(Rect::XYWH(100, 0, 80, 100))
                << SetHAlign(HAlign::Right)
                << SetFont(assets::OpenDyslexic64);
            m_ = addChild(new ui::Label{})
                << SetRect(Rect::XYWH(190, 0, 100, 100))
                << SetHAlign(HAlign::Left)
                << SetFont(assets::OpenDyslexic64);

            sep_ = addChild(new ui::Label{})
                << SetRect(Rect::XYWH(180, 0, 20, 100))
                << SetFont(assets::OpenDyslexic64)
                << SetText(":");

            refreshTime();

            h_->animate()
                << BreatheFg(h_, ui::Style::defaultStyle().accentFg(), ui::Style::defaultStyle().accentBg());

            root_.useBackgroundImage(false);

        }

    protected:

        void loop() override {
            ui::App<TinyTime>::loop();
            if (btnPressed(Btn::Left))
                swapActiveElement();
            if (btnPressed(Btn::Right))
                swapActiveElement();
            if (btnPressed(Btn::Up))
                moveUp();
            if (btnPressed(Btn::Down))
                moveDown();
            if (btnPressed(Btn::A))
                exit(TinyTime{hh_,mm_});
            if (btnPressed(Btn::B))
                exit();
        }

        void refreshTime() {
            h_->setText(STR(fillLeft(hh_, 2, '0')));
            m_->setText(STR(fillLeft(mm_, 2, '0')));
        }

        void swapActiveElement() {
            using namespace ui;
            Style const & style = ui::Style::defaultStyle();
            if (activeElement_ == 0) {
                activeElement_ = 1;
                h_->cancelAnimations();
                h_->setFg(style.defaultFg());
                m_->animate()
                    << BreatheFg(m_, style.accentFg(), style.accentBg());
            } else {
                activeElement_ = 0;
                m_->cancelAnimations();
                m_->setFg(style.defaultFg());
                h_->animate()
                    << BreatheFg(h_, style.accentFg(), style.accentBg());
            }
        }

        void moveUp() {
            switch (activeElement_) {
                case 0 :
                    if (++hh_ > 23)
                        hh_ = 0;
                    break;
                case 1 :
                    if (++mm_ > 59)
                        mm_ = 0;
                    break;
                default:
                    UNREACHABLE;
            }
            refreshTime();
        }

        void moveDown() {
            switch (activeElement_) {
                case 0 :
                    if (--hh_ > 23)
                        hh_ = 23;
                    break;
                case 1 :
                    if (--mm_ > 59)
                        mm_ = 59;
                    break;
                default:
                    UNREACHABLE;
            }
            refreshTime();
        }

    private:
       uint8_t hh_;
       uint8_t mm_;
       int activeElement_ = 0;

       ui::Image * icon_;
       ui::Label * h_;
       ui::Label * m_;
       ui::Label * sep_;
        
    }; // rckid::TimeDialog

} // namespace rckid