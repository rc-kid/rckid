#pragma once

#include <rckid/ui/app.h>
#include <rckid/ui/image.h>
#include <rckid/apps/launcher.h>

#include <assets/icons_64.h>

namespace rckid {

    class InfoDialog : public ui::App<void> {
    public:

        InfoDialog(String title, String message, ImageSource icon);

        static void info(String title, String message) {
            InfoDialog dlg{std::move(title), std::move(message), assets::icons_64::empty_box};
            dlg.run();
        }

        static void success(String title, String message) {
            InfoDialog dlg{std::move(title), std::move(message), assets::icons_64::empty_box};
            dlg.run();
        }

        static void error(String title, String message) {
            InfoDialog dlg{std::move(title), std::move(message), assets::icons_64::empty_box};
            dlg.run();
        }

    protected:
        void onLoopStart() override;

        void loop() override;

        ui::Image * icon_;
        ui::Label * title_;
        ui::Label * message_;

    }; 
}