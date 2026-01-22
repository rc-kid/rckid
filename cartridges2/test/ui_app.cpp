#include <rckid/ui/app.h>

class TestApp : public rckid::ui::App<void> {
public:
    TestApp() {
        using namespace rckid;
        using namespace rckid::ui;
        p_ = addChild(with(new Panel(Rect::XYWH(40, 40, 240, 160)))
            << SetBg(Color::Blue())
        );
        p_->addChild(with(new Panel(Rect::XYWH(-10, -10, 20, 20)))
            << SetBg(Color::Red())
        );
        p_->addChild(with(new Panel(Rect::XYWH(230, 150, 20, 20)))
            << SetBg(Color::Green())
        );
        p_->addChild(with(new Panel(Rect::XYWH(230, -10, 20, 20)))
            << SetBg(Color::Yellow())
        );
        p_->addChild(with(new Panel(Rect::XYWH(-10, 150, 20, 20)))
            << SetBg(Color::Cyan())
        );
        p_->addChild(with(new Panel(Rect::XYWH(50, 50, 20, 20)))
            << SetBg(Color::Magenta())
        );
    }
private:
    rckid::ui::Panel * p_;
};


int main() {
    rckid::initialize();
    TestApp app;
    app.run();
}