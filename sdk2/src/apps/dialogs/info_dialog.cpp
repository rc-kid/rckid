#include <assets/OpenDyslexic32.h>
#include <rckid/apps/dialogs/info_dialog.h>

namespace rckid {

    InfoDialog::InfoDialog(String title, String message, ImageSource icon):
        ui::App<void>{Rect::XYWH(0, 140, 320, 100)} 
    {
        using namespace ui;
        icon_ = addChild(new Image())
            << SetRect(Rect::XYWH(0, 0, 100, 100))
            << SetBitmap(std::move(icon))
            << SetVAlign(VAlign::Center)
            << SetHAlign(HAlign::Center);
        title_ = addChild(new Label())
            << SetRect(Rect::XYWH(100, 0, 220, 32))
            << SetText(title)
            << SetFont(assets::OpenDyslexic32)
            << SetVAlign(VAlign::Center)
            << SetHAlign(HAlign::Left);
        message_ = addChild(new Label())
            << SetRect(Rect::XYWH(100, 40, 220, 60))
            << SetText(message)
            << SetVAlign(VAlign::Center)
            << SetHAlign(HAlign::Left);
    }

    void InfoDialog::onLoopStart() {
        animate()
            << ui::FlyIn(icon_, 300, Point{-100, 0})
            << ui::FlyIn(title_, 300, Point{220, 0})->setDelayMs(50)
            << ui::FlyIn(message_, 300, Point{220, 0})->setDelayMs(150);
    }

    void InfoDialog::loop() {
        ui::App<void>::loop();
        if (btnPressed(Btn::B) || btnPressed(Btn::Down) || btnPressed(Btn::A) || btnPressed(Btn::Up)) {
            animate()
                << ui::FlyOut(icon_, 300, Point{-100, 0})->setDelayMs(150)
                << ui::FlyOut(title_, 300, Point{220, 0})->setDelayMs(100)
                << ui::FlyOut(message_, 300, Point{220, 0});
            waitUntilIdle();
            exit();
        }
    }



} // namespace rckid