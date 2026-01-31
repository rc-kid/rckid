#include <rckid/ui/app.h>
#include <rckid/ui/label.h>

class TestApp : public rckid::ui::App<void> {
public:
    TestApp() {
        using namespace rckid;
        using namespace rckid::ui;
        p_ = addChild(new Panel())
            << SetRect(Rect::XYWH(40, 40, 240, 160))
            << SetBg(Color::Blue());
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
        p_->addChild(new Panel())
            << SetRect(Rect::XYWH(50, 50, 20, 20))
            << Center()
            << SetBg(Color::Magenta());
        p_->addChild(new rckid::ui::Label())
            << SetRect(Rect::XYWH(0, 0, 240, 160))
            << CenterHorizontally()
            << SetText("Hello, RCKid UI!")
            << SetTextHAlign(HAlign::Center)
            << SetTextVAlign(VAlign::Center)
            << SetFont(rckid::assets::Iosevka16);
    }
private:
    rckid::ui::Panel * p_;
};


int main() {
    rckid::initialize();
    TestApp app;
    app.run();
}