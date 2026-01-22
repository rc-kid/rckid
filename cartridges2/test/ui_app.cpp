#include <rckid/ui/app.h>

class TestApp : public rckid::ui::App<void> {
public:
    TestApp() {
        using namespace rckid;
        using namespace rckid::ui;
        p_ = addChild(with(new Panel())
            << SetPosition(10, 10)
            << SetBg(Color::Blue())
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