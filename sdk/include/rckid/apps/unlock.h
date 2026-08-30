#pragma once

#include <rckid/ui/app.h>
#include <rckid/ui/label.h>
#include <rckid/ui/image.h>
#include <rckid/ui/animation.h>

#include <rckid/apps/dialogs/info_dialog.h>
#include <rckid/apps/dialogs/text_dialog.h>

#include <assets/icons_64.h>

namespace rckid {

    /** Simple unlock dialog. 
     
        Used to unlock the device when a password is set.
     */
    class Unlock : public ui::App<void> {
    public:

        String name() const override { return "Unlock"; }

        Unlock(String expected): 
            expected_(expected) 
        {
            using namespace ui;
            icon_ = addChild(new Image())
                << SetRect(Rect::XYWH(0, 60, 320, 64))
                << SetBitmap(assets::icons_64::poo);
        }

    protected:

        void onLoopStart() override {
            root_.flyIn();
        }

        void loop() override {
            using namespace ui;
            waitUntilIdle();
            App<void>::loop();
            auto pwd = App::run<TextDialog>("");
            if (pwd) {
                if (pwd.value() == expected_) {
                    root_.flyOut();
                    waitUntilIdle();
                    exit();
                    return;
                } else {
                    InfoDialog::error("Wrong password", "The password you entered is incorrect");
                }
            }
        }

    private:

        ui::Image * icon_ = nullptr;

        String expected_;

    }; // rckid::Unlock

} // namespace rckid