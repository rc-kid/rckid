#pragma once

#include <rckid/ui/app.h>
#include <rckid/ui/label.h>
#include <rckid/ui/image.h>
#include <assets/Iosevka24.h>

namespace rckid {

    /** Displays information about the SDK & device. 
     
        Can also be used to launch specific settings, such as entering parent mode, etc.
     */
    class About : public ui::App<void> {
    public:
        String name() const override { return "About"; }

        About() {
            using namespace ui;
            icon_ = addChild(new ui::Image{})
                << SetBitmap(assets::icons_64::info)
                << SetRect(Rect::XYWH(0, 20, 96, 64));
            name_ = addChild(new ui::Label{})
                << SetText("RCKid mkIII")
                << SetRect(Rect::XYWH(95, 15, 220, 64))
                << SetFont(assets::OpenDyslexic64);
            version_ = addChild(new ui::Label{})
                << SetText("version 1.0.0")
                << SetRect(Rect::XYWH(95, 64, 220, 16));

            TinyTime budget{pim::remainingBudget()};
            budget_ = addChild(new ui::Label{})
                << SetFont(assets::Iosevka24)
                << SetText(STR("Budget: " << budget))
                << SetRect(Rect::XYWH(45, 90, 320, 24));
        }
        
    protected:

        void onLoopStart() override {
            ui::App<void>::onLoopStart();
            root_.flyIn();
        }

        void loop() override {
            ui::App<void>::loop();
            if (btnPressed(Btn::B) || btnPressed(Btn::Down)) {
                exit();
                // wait for idle to make sure we are exiting from known state
                waitUntilIdle();
                root_.flyOut();
                waitUntilIdle();
            }
        }

    private:
        ui::Image * icon_;
        ui::Label * name_;
        ui::Label * version_;
        ui::Label * budget_;
    }; // rckid::About

} // namespace rckid