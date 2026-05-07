#pragma once

#include <rckid/ui/app.h>
#include <rckid/ui/label.h>
#include <rckid/ui/image.h>

#include <assets/icons_64.h>
#include <assets/OpenDyslexic64.h>
#include <assets/OpenDyslexic32.h>

namespace rckid {

    /** Dialog which allows entering date. 
     
        Shows day, month, and year and calculates day in week. 
     */
    class DateDialog : public ui::App<TinyDate> {
    public:

        String name() const override { return "Select date"; }

        DateDialog(TinyDate date = TinyDate{1, 1, 2026}):
            ui::App<TinyDate>{Rect::XYWH(0, 140, 320, 100)},
            dd_{date.day()},
            mm_{date.month()},
            yy_{date.year()}
        {
            using namespace ui;

            icon_ = addChild(new ui::Image{})
                << SetRect(Rect::XYWH(0, 0, 90, 100))
                << SetBitmap(assets::icons_64::poo);

            d_ = addChild(new ui::Label{})
                << SetRect(Rect::XYWH(70, 20, 60, 76))
                << SetHAlign(HAlign::Right)
                << SetFont(assets::OpenDyslexic64);
            m_ = addChild(new ui::Label{})
                << SetRect(Rect::XYWH(135, 20, 60, 76))
                << SetHAlign(HAlign::Right)
                << SetFont(assets::OpenDyslexic64);
            y_ = addChild(new ui::Label{})
                << SetRect(Rect::XYWH(215, 20, 90, 76))
                << SetFont(assets::OpenDyslexic64);

            dayOfWeek_ = addChild(new ui::Label{})
                << SetRect(Rect::XYWH(70, 14, 60, 32))
                << SetHAlign(HAlign::Right)
                << SetFont(assets::OpenDyslexic32);
            month_ = addChild(new ui::Label{})
                << SetRect(Rect::XYWH(135, 14, 60, 32))
                << SetHAlign(HAlign::Right)
                << SetFont(assets::OpenDyslexic32);

            sep1_ = addChild(new ui::Label{})
                << SetRect(Rect::XYWH(130, 20, 20, 76))
                << SetFont(assets::OpenDyslexic64)
                << SetText("/");
            sep2_ = addChild(new ui::Label{})
                << SetRect(Rect::XYWH(195, 20, 20, 76))
                << SetFont(assets::OpenDyslexic64)
                << SetText("/");
            
            refreshDate();

            d_->animate()
                << BreatheFg(d_, ui::Style::defaultStyle().accentFg(), ui::Style::defaultStyle().accentBg());

            root_.useBackgroundImage(false);
        }

    private:

        void loop() override {
            ui::App<TinyDate>::loop();
            if (btnPressed(Btn::Left))
                setActiveElement(activeElement_ - 1);
            if (btnPressed(Btn::Right))
                setActiveElement(activeElement_ + 1);
            if (btnPressed(Btn::Up))
                moveUp();
            if (btnPressed(Btn::Down))
                moveDown();
            if (btnPressed(Btn::A))
                exit(TinyDate{dd_,mm_,yy_});
            if (btnPressed(Btn::B))
                exit();
        }

        void refreshDate() {
            d_->setText(STR(dd_));
            m_->setText(STR(mm_));
            y_->setText(STR(yy_));
            dayOfWeek_->setText(STR(TinyDate::dayOfWeek(dd_, mm_, yy_)).substr(0, 3));
            month_->setText(STR(static_cast<TinyDate::Month>(mm_)).substr(0, 3));
            
        }

        ui::Label * activeElement() {
            switch (activeElement_) {
                case 0 : return d_;
                case 1 : return m_;
                case 2 : return y_;
                default:
                    UNREACHABLE;
            }
        }

        void setActiveElement(int index) {
            using namespace ui;
            if (index < 0)
                index = 2;
            if (index > 2)
                index = 0;
            Style const & style = ui::Style::defaultStyle();
            Label * x = activeElement();
            x->cancelAnimations();
            x->setFg(style.defaultFg());
            activeElement_ = index;
            x = activeElement();
            x->animate()
                << BreatheFg(x, style.accentFg(), style.accentBg());
        }

        void moveUp() {
            switch (activeElement_) {
                case 0:
                    if (++dd_ > TinyDate::daysInMonth(mm_, yy_))
                        dd_ = 1;
                    break;
                case 1:
                    if (++mm_ > 12)
                        mm_ = 1;
                    if (dd_ > TinyDate::daysInMonth(mm_, yy_))
                        dd_ = TinyDate::daysInMonth(mm_, yy_);
                    break;
                case 2:
                    if (++yy_ > 4095)
                        yy_ = 0;
                    if (dd_ > TinyDate::daysInMonth(mm_, yy_))
                        dd_ = TinyDate::daysInMonth(mm_, yy_);
                    break;                    
            }
            refreshDate();
        }

        void moveDown() {
            switch (activeElement_) {
                case 0:
                    if (--dd_ == 0)
                        dd_ = TinyDate::daysInMonth(mm_, yy_);
                    break;
                case 1:
                    if (--mm_ == 0)
                        mm_ = 12;
                    if (dd_ > TinyDate::daysInMonth(mm_, yy_))
                        dd_ = TinyDate::daysInMonth(mm_, yy_);
                    break;
                case 2:
                    if (--yy_ > 4095)
                        yy_ = 0;
                    if (dd_ > TinyDate::daysInMonth(mm_, yy_))
                        dd_ = TinyDate::daysInMonth(mm_, yy_);
                    break;                    
            }
            refreshDate();
        }

        uint8_t dd_;
        uint8_t mm_;
        uint16_t yy_;
        int activeElement_ = 0;

        ui::Image * icon_;
        ui::Label * d_;
        ui::Label * m_;
        ui::Label * y_;
        ui::Label * dayOfWeek_;
        ui::Label * month_;
        ui::Label * sep1_;
        ui::Label * sep2_;
    }; // DateDialog


} // namespace rckid