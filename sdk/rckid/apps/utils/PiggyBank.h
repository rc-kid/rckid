#pragma once

#include "../../app.h"
#include "../../ui/form.h"
#include "../../ui/label.h"
#include "../../ui/image.h"
#include "../../assets/fonts/OpenDyslexic128.h"
#include "../../assets/fonts/OpenDyslexic64.h"
#include "../../assets/icons_24.h"
#include "../dialogs/TimeDialog.h"
#include "../dialogs/DateDialog.h"
#include "../dialogs/InfoDialog.h"
#include "../dialogs/NumberDialog.h"

namespace rckid {
    class PiggyBank : public ui::Form<void> {
    private:
        struct Settings {
            int32_t value = 0;
            int32_t allowance = 0;
            TinyDate lastTopUp;
        };

    public:

        String name() const override { return "PiggyBank"; }

          PiggyBank(): 
                ui::Form<void>{}
            {
                settings_ = loadSettings();

                cash_ = g_.addChild(new ui::Label{Rect::XYWH(0, 30, 320, 130), ""});
                topupIcon_ = g_.addChild(new ui::Image{Rect::XYWH(130, 180, 24, 24), Icon{assets::icons_24::money_bag}});
                topUp_ = g_.addChild(new ui::Label{});
                cash_->setFont(Font::fromROM<assets::OpenDyslexic128>());
                cash_->setHAlign(HAlign::Center);
                topUp_->setFont(Font::fromROM<assets::OpenDyslexic32>());
                // topup countdown
                TinyDate now = timeNow().date;
                TinyDate topUp{1, static_cast<uint8_t>((now.month() == 12) ? 1 : (now.month() + 1)), now.year()};
                uint32_t daysTillTopUp = now.daysTillNextAnnual(topUp);
                topUp_->setText(STR("+" << settings_.allowance << " in " << daysTillTopUp << " days"));
                Coord w = topUp_->textWidth() + 24;
                topupIcon_->setPos(160 - (w / 2), 144);
                topUp_->setPos(160 - (w / 2) + 24, 140);

                cash_->setText(STR(settings_.value));
                contextMenu_.add(ui::ActionMenu::Item("Update", [this]() {
                    auto n = App::run<NumberDialog>("Enter amount");
                    if (n.has_value()) {
                        settings_.value += n.value();
                        cash_->setText(STR(settings_.value));
                        saveSettings(settings_);
                    }
                }));
                contextMenu_.add(ui::ActionMenu::Item("Set monthly value", [this]() {
                    auto n = App::run<NumberDialog>("Allowance");
                    if (n.has_value()) {
                        settings_.allowance = n.value();
                        TinyDate now = timeNow().date;
                        TinyDate topUp{1, static_cast<uint8_t>((now.month() == 12) ? 1 : (now.month() + 1)), now.year()};
                        uint32_t daysTillTopUp = now.daysTillNextAnnual(topUp);
                        topUp_->setText(STR("+" << settings_.allowance << " in " << daysTillTopUp << " days"));
                        saveSettings(settings_);
                    }
                }));
            }

        ~PiggyBank() override {
            saveSettings(settings_);
        }

        static int32_t updateAllowance(int32_t by) {
            Settings settings = loadSettings();
            settings.value += by;
            saveSettings(settings);
            return settings.value;
        }

    protected:

        static constexpr char const * HOME_FOLDER = "/apps/PiggyBank";
        static constexpr char const * SETTINGS_PATH = "/apps/PiggyBank/settings.ini";

        void update() override {
            ui::Form<void>::update();

            if (btnPressed(Btn::B) || btnPressed(Btn::Down))
                exit();

            // piggy bank actions only work in parent mode
            if (btnPressed(Btn::Select) && parentMode()) {
                auto action = App::run<PopupMenu<ui::Action>>(&contextMenu_);
                if (action.has_value())
                    action.value()();
            }
        }

        void draw() override {
            ui::Form<void>::draw();
        }

        static void saveSettings(Settings const & s) {
            if (fs::isMounted() && fs::createFolders(HOME_FOLDER)) {
                ini::Writer ini{fs::fileWrite(SETTINGS_PATH)};
                ini.writeSection("settings");
                ini.writeValue("value", STR(s.value));
                ini.writeValue("allowance", STR(s.allowance));
                ini.writeValue("lastTopUp", STR(s.lastTopUp.day() << "/" << s.lastTopUp.month() << "/" << s.lastTopUp.year()));
            }
        }

        static Settings loadSettings() {
            Settings result;
            result.lastTopUp = timeNow().date;
            if (! fs::isMounted())
                return result;
            if (! fs::exists(SETTINGS_PATH))
                return result;
            {
                ini::Reader ini{fs::fileRead(SETTINGS_PATH)};
                while (auto section = ini.nextSection()) {
                    if (section.value() == "settings") {
                        while (auto kv = ini.nextValue()) {
                            if (kv->first == "value") {
                                result.value = std::atoi(kv->second.c_str());
                            } else if (kv->first == "allowance") {
                                result.allowance = std::atoi(kv->second.c_str());
                            } else if (kv->first == "lastTopUp") {
                                result.lastTopUp.setFromString(kv->second);
                            } else {
                                LOG(LL_ERROR, "Unknown setting: " << kv->first);}
                        }
                    } else {
                        LOG(LL_ERROR, "Invalid settings section: " << section.value());
                    }
                }
            }
            // check if we should top up automatically
            if (result.allowance > 0) {
                TinyDate now = timeNow().date;
                bool changed = false;
                bool topup = false;
                if (now.year() >= 2025) {
                    if (result.lastTopUp.year() >= 2025) {
                        while (result.lastTopUp.year() < now.year() || result.lastTopUp.month() < now.month()) {
                            result.value += result.allowance;
                            result.lastTopUp.incMonth();
                            changed = true;
                            topup = true;
                        } 
                    } else {
                        changed = true;
                    }
                }
                if (changed) {
                    result.lastTopUp = now;
                    saveSettings(result);
                    if (topup)
                        App::run<InfoDialog>(Icon{assets::icons_64::money_bag},"Top-up", STR("Hooray, you now have " << result.value));
                }
            }
            // and return the result
            return result;
        }

    private:

        ui::Label * cash_;
        ui::Label * topUp_;
        ui::Image * topupIcon_;
        ui::ActionMenu contextMenu_; 

        Settings settings_;
    }; // rckid::Clock

} // namespace rckid