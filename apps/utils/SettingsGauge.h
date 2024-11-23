#pragma once 

#include <rckid/app.h>
#include <rckid/graphics/canvas.h>
#include <rckid/ui/gauge.h>
#include <rckid/assets/icons64.h>
#include <rckid/assets/fonts/OpenDyslexic48.h>
#include <rckid/ui/header.h>

namespace rckid {

    class SettingsGauge : public GraphicsApp<Canvas<ColorRGB>> {
    public:
        static void run(int min, int max, int value, int step, char const * title, Bitmap<ColorRGB> && icon, std::function<void(int)> onChange) {
            SettingsGauge * app = ARENA(new SettingsGauge{title, std::move(icon), onChange});
            app->gauge_.setMin(min);
            app->gauge_.setMax(max);
            app->gauge_.setValue(value);
            app->gauge_.setStep(step);
            app->loop();
            delete app;
        }

    protected:

        SettingsGauge(char const * title, Bitmap<ColorRGB> && icon, std::function<void(int)> onChange): GraphicsApp{ARENA(Canvas<ColorRGB>{320, 240})},
            onChange_{onChange},
            icon_{std::move(icon)}, 
            title_{title},
            titleWidth_{assets::font::OpenDyslexic48::font.textWidth(title)} {
        }

        void update() override {
            if (gauge_.update())
                onChange_(gauge_.value());
            GraphicsApp::update();
        }

        void draw() override {
            NewArenaScope _{};
            g_.fill();
            gauge_.drawOn(g_, Rect::XYWH(10, 140, 300, 5));
            Font const & f = assets::font::OpenDyslexic48::font;
            int x = 160 - (titleWidth_ + 5 + icon_.width()) / 2;
            int y = 140 - icon_.height() - 10;
            g_.blit(Point{x, y}, icon_);  
            g_.text(x + icon_.width() + 5, y + (icon_.height() - f.size) / 2, f, color::White) << title_;
            Header::drawOn(g_);
        }

    private:
        Gauge gauge_;
        std::function<void(int)> onChange_;
        Bitmap<ColorRGB> icon_;
        char const * title_;
        int titleWidth_ = 0;

    }; // rckid::SettingsGauge

    void setBrightness() {
        ArenaScope _{};
        SettingsGauge::run(0, 255, displayBrightness(), 16, "Brightness", Bitmap<ColorRGB>::fromImage(PNG::fromBuffer(assets::icons64::brightness)), [](int value) {
            displaySetBrightness(value);
        });
    }

    void setVolume() {
        ArenaScope _{};
        SettingsGauge::run(0, 10, audioVolume(), 1, "volume", Bitmap<ColorRGB>::fromImage(PNG::fromBuffer(assets::icons64::high_volume)), [](int value) { audioSetVolume(value); });
    }

} // namespace rckid 