#include <rckid/ui/app.h>
#include <rckid/ui/label.h>
#include <rckid/ui/image.h>
#include <rckid/ui/animation.h>
#include <assets/images.h>

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
            << SetBitmap(ImageSource{assets::logo});
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
        with(ax_)
            << SetAnimationOnUpdate([this](FixedRatio progress) {
                Coord x = static_cast<rckid::Coord>(progress * (240 - 20));
                ui::with(ap_) << ui::SetPosition(x, ap_->rect().y);
            })
            << SetAnimationMode(Animation::Mode::Oscillate)
            << SetDurationMs(2000)
            << Start();
        with(ay_)
            << SetAnimationOnUpdate([this](FixedRatio progress) {
                Coord y = static_cast<rckid::Coord>(progress * (160 - 20));
                ui::with(ap_) << ui::SetPosition(ap_->rect().x, y);
            })
            << SetAnimationMode(Animation::Mode::Oscillate)
            << SetDurationMs(2000)
            << Start();
    }

private:
    rckid::ui::Panel * p_;
    rckid::ui::Panel * ap_;
    rckid::ui::Animation ax_;
    rckid::ui::Animation ay_;
};


int main() {
    rckid::initialize();
    TestApp app;
    app.run();
}