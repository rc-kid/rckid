#pragma once

#include <rckid/ui/app.h>
#include <rckid/ui/image.h>
#include <rckid/apps/launcher.h>

#include <assets/icons_64.h>

namespace rckid {

    class InfoDialog : public ui::App<void> {
    public:

        String name() const override { return "Info Dialog"; }

        InfoDialog(String title, String message, ImageSource icon);

        static void info(String title, String message) {
            InfoDialog dlg{std::move(title), std::move(message), assets::icons_64::info};
            dlg.setBg(Color::RGB(0, 0, 64));
            dlg.run();
        }

        static void success(String title, String message) {
            InfoDialog dlg{std::move(title), std::move(message), assets::icons_64::happy_face};
            dlg.setBg(Color::RGB(0, 64, 0));
            dlg.run();
        }

        static void error(String title, String message) {
            InfoDialog dlg{std::move(title), std::move(message), assets::icons_64::sad_face};
            dlg.setBg(Color::RGB(64, 0, 0));
            dlg.run();
        }

        void setBg(Color bg) {
            root_.setBg(bg);
        }

    protected:
        void onLoopStart() override;

        void loop() override;

        ui::Image * icon_;
        ui::Label * title_;
        ui::Label * message_;

    }; 
}