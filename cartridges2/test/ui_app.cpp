#include <rckid/ui/app.h>
#include <rckid/ui/label.h>
#include <rckid/ui/image.h>
#include <rckid/ui/animation.h>
#include <rckid/ui/animation_effects.h>
#include <rckid/ui/carousel.h>
#include <assets/images.h>
#include <assets/icons_64.h>

#include <rckid/apps/steps.h>

using namespace rckid;

class TestApp : public rckid::ui::App<void> {
public:
    TestApp() {
        using namespace rckid;
        using namespace rckid::ui;
        p_ = addChild(new Panel())
            << SetRect(Rect::XYWH(40, 40, 240, 160))
            << SetBg(Color::Blue());
        p_->addChild(new Image())
            << SetRect(Rect::XYWH(0, 0, 240, 160))
            //<< SetVisibility(false)
            << SetBitmap(assets::images::logo);
        c_ = p_->addChild(new Carousel()) 
            << SetRect(Rect::XYWH(0, 60, 240, 100));
        p_->addChild(new Panel())
            << SetRect(Rect::XYWH(-10, -10, 20, 20))
            << SetBg(Color::Red());
        p_->addChild(new Panel())
            << SetRect(Rect::XYWH(230, 150, 20, 20))
            << SetBg(Color::Green());
        p_->addChild(new Panel())
            << SetRect(Rect::XYWH(230, -10, 20, 20))
            << SetBg(Color::Yellow());
        p_->addChild(new Panel())
            << SetRect(Rect::XYWH(-10, 150, 20, 20))
            << SetBg(Color::Cyan());
        ap_ = p_->addChild(new Panel())
            << SetRect(Rect::XYWH(50, 50, 20, 20))
            << Center()
            << SetBg(Color::Magenta());
        p_->addChild(new rckid::ui::Label())
            << SetRect(Rect::XYWH(0, 0, 240, 160))
            << CenterHorizontally()
            << SetText("Hello, RCKid UI!")
            << SetHAlign(HAlign::Center)
            << SetVAlign(VAlign::Center)
            << SetFont(rckid::assets::Iosevka16);
        ap_->animate()
            << SetAnimationMode(Animation::Mode::Oscillate)
            << SetDurationMs(3000)
            << MoveHorizontally(0, 240 - 20);
        ap_->animate()
            << SetAnimationMode(Animation::Mode::Oscillate)
            << SetDurationMs(2000)
            << MoveVertically(0, 160 - 20);
        c_->set("Empty", assets::icons_64::empty_box);
    }

private:
    rckid::ui::Panel * p_;
    rckid::ui::Panel * ap_;
    rckid::ui::Carousel * c_;
};


int main() {
    rckid::initialize();
    //Steps app;
    TestApp app;
    app.run();
}