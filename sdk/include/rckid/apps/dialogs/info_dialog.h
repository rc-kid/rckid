#pragma once

#include <rckid/ui/app.h>
#include <rckid/ui/image.h>
#include <rckid/apps/launcher.h>

#include <assets/icons_64.h>

namespace rckid {

    class InfoDialog : public ui::App<void> {
    public:

        enum class Kind {
            Default,
            Error,
            Success,
            Info
        };

        String name() const override { return "Info Dialog"; }

        InfoDialog(String title, String message, ImageSource icon, Kind kind = Kind::Default);

        static void info(String title, String message) {
            LOG(LL_INFO, title << ": " << message);
            App::run<InfoDialog>(std::move(title), std::move(message), assets::icons_64::info, Kind::Info);
        }

        static void success(String title, String message) {
            LOG(LL_INFO, title << ": " << message);
            App::run<InfoDialog>(std::move(title), std::move(message), assets::icons_64::happy_face, Kind::Success);
        }

        static void error(String title, String message) {
            LOG(LL_ERROR, title << ": " << message);
            App::run<InfoDialog>(std::move(title), std::move(message), assets::icons_64::sad_face, Kind::Error);
        }

        void setBg(Color bg) {
            root_.setBg(bg);
        }

        static Color bgColorFor(Kind kind) {
            switch (kind) {
                case Kind::Error:
                    return Color::Red().withBrightness(48);
                case Kind::Success:
                    return Color::Green().withBrightness(48);
                case Kind::Info:
                    return Color::Blue().withBrightness(48);
                case Kind::Default:
                default:
                    return ui::Style::defaultStyle().defaultBg();
            }
        }

        static Color textColorFor(Kind kind) {
            switch (kind) {
                case Kind::Error:
                case Kind::Success:
                case Kind::Info:
                    return Color::White();
                case Kind::Default:
                default:
                    return ui::Style::defaultStyle().defaultFg();
            }
        }

    protected:
        void onLoopStart() override;

        void loop() override;

        ui::Image * icon_;
        ui::Label * title_;
        ui::MultiLabel * message_;

    }; 
}