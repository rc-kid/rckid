#pragma once

#include <rckid/ui/app.h>
#include <rckid/ui/image.h>
#include <rckid/apps/launcher.h>

#include <assets/icons_64.h>

namespace rckid {

    class InfoDialog : public ui::App<void> {
    public:

        String name() const override { return "Info Dialog"; }

        InfoDialog(String title, String message, ImageSource icon, ui::Theme theme = ui::Theme::Default);

        static void info(String title, String message) {
            LOG(LL_INFO, title << ": " << message);
            App::run<InfoDialog>(std::move(title), std::move(message), assets::icons_64::info, ui::Theme::Info);
        }

        static void success(String title, String message) {
            LOG(LL_INFO, title << ": " << message);
            App::run<InfoDialog>(std::move(title), std::move(message), assets::icons_64::happy_face, ui::Theme::Success);
        }

        static void error(String title, String message) {
            LOG(LL_ERROR, title << ": " << message);
            App::run<InfoDialog>(std::move(title), std::move(message), assets::icons_64::sad_face, ui::Theme::Error);
        }

        void setBg(Color bg) {
            root_.setBg(bg);
        }

    protected:
        void onLoopStart() override;

        void loop() override;

        ui::Image * icon_;
        ui::Label * title_;
        ui::MultiLabel * message_;

    }; 
}