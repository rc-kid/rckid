#pragma once

#include "../../app.h"
#include "../../ui/form.h"
#include "../../ui/label.h"
#include "../../ui/image.h"
#include "../../assets/icons_64.h"
#include "../../assets/fonts/OpenDyslexic64.h"

namespace rckid {

    /** Simple Steps counter.
     */
    class Steps : public ui::Form<void> {
    public:

        String name() const override { return "Steps"; }

        Steps():
            ui::Form<void>{}
        {
            icon_ = g_.addChild(new ui::Image{Icon{assets::icons_64::footprint}});
            status_ = g_.addChild(new ui::Label{Rect::XYWH(0, 130, 320, 64), STR(pedometerCount())});
            status_->setFont(Font::fromROM<assets::OpenDyslexic64>());
            icon_->setTransparent(true);
            icon_->setPos(160 - icon_->width() / 2, 60);
            status_->setHAlign(HAlign::Center);
            loadSettings();
            saveValueIfNeeded();
        }

        ~Steps() override {
            saveSettings();
        }

        static void onHeartbeat() {
            // just start the app, which in constructor verifies if saving the data is needed
            Steps app{};
        }

    protected:

        void update() override {
            ui::Form<void>::update();
            if (btnPressed(Btn::B) || btnPressed(Btn::Down))
                exit();
        }

        void onSecondTick() override {
            status_->setText(STR(pedometerCount()));
        }

        void loadSettings() {
            auto r = settingsReader();
            if (r.has_value()) {
                auto & ini = r.value();
                while (auto section = ini.nextSection()) {
                    if (section.value() == "Settings") {
                        while (auto kv = ini.nextValue()) {
                            if (kv->first == "currentDay") {
                                if (! currentDay_.setFromString(kv->second.c_str()))
                                    LOG(LL_ERROR, "Invalid date format: " << kv->second);
                            } else {
                                LOG(LL_ERROR, "Unknown key in Steps settings: " << kv->first);
                            }
                        }
                    } else {
                        LOG(LL_ERROR, "Unknown section in Steps settings: " << section.value());
                    }
                }
            } else {
                currentDay_ = timeNow().date;
                LOG(LL_INFO, "currentDay_ is set to " << currentDay_);
            }
        }

        void saveSettings() {
            auto w = settingsWriter();
            if (w.has_value()) {
                auto & ini = w.value();
                ini.writeSection("Settings");
                ini.writeValue("currentDay", STR(currentDay_));
            }
        }

        void saveCurrentValue() {
            fs::FileWrite f{fs::fileAppend(fs::join(homeFolder(), "log.dat"))};
            if (f.good()) {
                LOG(LL_INFO, "Steps log save: " << currentDay_ << ": " << pedometerCount() << " steps");
                serialize(f, currentDay_);
                serialize(f, pedometerCount());
            }
        }

        void saveValueIfNeeded() {
            TinyDate today = timeNow().date;
            if (today != currentDay_) {
                saveCurrentValue();
                currentDay_ = today;
                pedometerReset();
            }
        }

    private:
        ui::Image * icon_;
        ui::Label * status_;

        // day for which the pedometer count is active
        TinyDate currentDay_;

    }; // Steps
} // namespace rckid