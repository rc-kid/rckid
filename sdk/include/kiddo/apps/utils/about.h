#pragma once

#include <kiddo/ui/app.h>
#include <kiddo/ui/label.h>
#include <kiddo/ui/image.h>
#include <kiddo/apps/dialogs/popup_menu.h>

#include <assets/Iosevka24.h>

namespace kiddo {

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
                << SetText("kidDo mkIII")
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

        void loop() override {
            ui::App<void>::loop();
            if (btnPressed(Btn::B) || btnPressed(Btn::Down))
                return exit();
            if (btnPressed(Btn::Select)) {
                PopupMenu::run(
                    debug::debugMode() ? 
                        ui::MenuItem("Leave Debug Mode", assets::icons_16::ladybug, [](){
                            debug::setDebugMode(false);
                            InfoDialog::info("Debug mode", "Debug mode disabled");
                        })                    
                    : 
                        ui::MenuItem("Enter Debug Mode", assets::icons_16::ladybug, [](){
                            debug::setDebugMode(true);
                            InfoDialog::info("Debug mode", "Debug mode enabled");
                        })
                );
            }
        }

    private:
        ui::Image * icon_;
        ui::Label * name_;
        ui::Label * version_;
        ui::Label * budget_;
    }; // kiddo::About

} // namespace kiddo