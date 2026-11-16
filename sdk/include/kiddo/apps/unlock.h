#pragma once

#include <kiddo/ui/app.h>
#include <kiddo/ui/label.h>
#include <kiddo/ui/image.h>
#include <kiddo/ui/animation.h>

#include <kiddo/apps/dialogs/info_dialog.h>
#include <kiddo/apps/dialogs/text_dialog.h>
#include <kiddo/apps/dialogs/popup_menu.h>

#include <assets/icons_64.h>

namespace kiddo {

    /** Simple unlock dialog. 
     
        Used to unlock the device when a password is set. 
        
        In its context menu, allows setting the parent mode, which enables bypassing the device lock if parent password is entered.
     */
    class Unlock : public ui::App<bool> {
    public:

        String name() const override { return "Unlock"; }

        Unlock(String expected, bool unlimitedRetries = true): 
            expected_(expected), 
            unlimitedRetries_(unlimitedRetries)
        {
            using namespace ui;
            icon_ = addChild(new Image())
                << SetRect(Rect::XYWH(0, 60, 320, 64))
                << SetBitmap(assets::icons_64::lock);
        }

    protected:

        void loop() override {
            using namespace ui;
            waitUntilIdle();
            App<bool>::loop();
            // if select button is pressed, show first the options menu
            if (btnDown(Btn::Select)) {
                PopupMenu::run(
                    // poo
                    ui::MenuItem("Enter Parent Mode", assets::icons_16::bookmark, [](){
                        pim::enterParentMode();
                    })                    
                );
                if (pim::parentMode())
                    return exit(true);
            }
            auto pwd = App::run<TextDialog>("");
            if (pwd) {
                if (pwd.value() == expected_) {
                    return exit(true);
                } else {
                    InfoDialog::error("Wrong password", "The password you entered is incorrect");
                }
            }
            if (! unlimitedRetries_)
                return exit(false);
        }

    private:

        ui::Image * icon_ = nullptr;

        String expected_;
        bool unlimitedRetries_;

    }; // kiddo::Unlock

} // namespace kiddo