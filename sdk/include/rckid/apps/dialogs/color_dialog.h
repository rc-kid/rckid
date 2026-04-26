#pragma once

#include <rckid/ui/app.h>
#include <rckid/ui/image.h>
#include <rckid/ui/progress_bar.h>
#include <rckid/ui/focus_rect.h>
#include <rckid/apps/launcher.h>

#include <assets/icons_64.h>

namespace rckid {

    /** Very basic color picker.  
     
        TODO would be good to add other than RGB color schemes as well. Or maybe even previously used colors, etc. 
     */
    class ColorDialog : public ui::App<Color> {
    public:
        
        String name() const override { return "ColorDialog"; }

        ColorDialog(Color color):
            ui::App<Color>(Rect::XYWH(0, 140, 320, 100))
        {
            using namespace ui;
            icon_ = addChild(new ui::Image{})
                << SetRect(Rect::XYWH(220, 0, 100, 100))
                << SetBitmap(assets::icons_64::light);
            r_ = addChild(new ui::ProgressBar{})
                << SetRect(Rect::XYWH(10, 10, 210, 20))
                << SetFg(Color::Red())
                << SetBg(Color::Red().withBrightness(48))
                << SetRange(0, 31)
                << SetValue(color.r >> 3);
            g_ = addChild(new ui::ProgressBar{})
                << SetRect(Rect::XYWH(10, 40, 210, 20))
                << SetFg(Color::Green())
                << SetBg(Color::Green().withBrightness(48))
                << SetRange(0, 63)
                << SetValue(color.g >> 2);
            b_ = addChild(new ui::ProgressBar{})
                << SetRect(Rect::XYWH(10, 70, 210, 20))
                << SetFg(Color::Blue())
                << SetBg(Color::Blue().withBrightness(48))
                << SetRange(0, 31)
                << SetValue(color.b >> 3);
            focus_ = addChild(new ui::FocusRect{});


            focus_->showAround(r_, /* animate */ false);

            root_.useBackgroundImage(false);
            root_.setBg(color);
        }

    protected:

        void onLoopStart() override {
            ui::App<Color>::onLoopStart();
            root_.flyIn();
            current_ = r_;
        }

        void loop() override {
            ui::App<Color>::loop();
            if (btnPressed(Btn::B)) {
                exit();
                // wait for idle to make sure we are exiting from known state
                waitUntilIdle();
                root_.flyOut();
                waitUntilIdle();
            }
            if (btnPressed(Btn::A)) {
                exit(root_.bg());
                // wait for idle to make sure we are exiting from known state
                waitUntilIdle();
                root_.flyOut();
                waitUntilIdle();
            }
            if (btnPressed(Btn::Left)) {
                current_->changeValueBy(-1);
                updateColor();
            }
            if (btnPressed(Btn::Right)) {
                current_->changeValueBy(1);
                updateColor();
            }
            if (btnPressed(Btn::Up)) {
                if (current_ == r_)
                    setCurrent(b_);
                else if (current_ == g_)
                    setCurrent(r_);
                else
                    setCurrent(g_);
            }
            if (btnPressed(Btn::Down)) {
                if (current_ == r_)
                    setCurrent(g_);
                else if (current_ == g_)
                    setCurrent(b_);
                else 
                    setCurrent(r_);
            }
        }

        void setCurrent(ui::ProgressBar * value) {
            current_ = value;
            focus_->showAround(value);
            // TODO move focus rectangle
        }

        void updateColor() {
            Color c = Color::RGB(
                (r_->value() << 3) + (r_->value() >> 2),
                (g_->value() << 2) + (g_->value() >> 4),
                (b_->value() << 3) + (b_->value() >> 2)
            );
            root_.setBg(c);
        }

    private:
        ui::Image * icon_;
        ui::ProgressBar * r_;
        ui::ProgressBar * g_;
        ui::ProgressBar * b_;
        ui::FocusRect * focus_;

        ui::ProgressBar * current_;

    }; // rckid::ColorDialog

} // namespace rckid 