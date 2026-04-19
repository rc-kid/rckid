#pragma once

#include <rckid/ui/app.h>
#include <rckid/ui/label.h>
#include <rckid/ui/image.h>
#include <rckid/ui/animation.h>

#include <rckid/apps/dialogs/info_dialog.h>

#include <assets/OpenDyslexic64.h>
#include <assets/OpenDyslexic32.h>
#include <assets/icons_64.h>
#include <assets/icons_24.h>

namespace rckid {

    /** Piggy Bank.
     
     */
    class PiggyBank : public ui::App<void> {
    public:

        String name() const override { return "PiggyBank"; }

        PiggyBank() {
            using namespace ui;
            // check if we have any previous settings
            auto f = fs::readFile("state.ini");
            if (f != nullptr) {
                ini::Reader reader{*f};
                state_.load(reader);
                checkTopUp();
            }
            // when not available, show error message and exit the app
            icon_ = addChild(new Image())
                << SetRect(Rect::XYWH(0, 50, 320, 64))
                << SetBitmap(assets::icons_64::piggy_bank);
            ballance_ = addChild(new Label())
                << SetRect(Rect::XYWH(0, 120, 320, 64))
                << SetText("300")
                << SetFont(assets::OpenDyslexic64)
                << SetHAlign(HAlign::Center);
            nextTopUp_ = addChild(new Label())
                << SetRect(Rect::XYWH(0, 0, 320, 32))
                << SetHAlign(HAlign::Left)
                << SetText(STR(state_.allowance << " in " << daysTillTopUp_ << " days"))
                << SetFont(assets::OpenDyslexic32);
            topUp_ = addChild(new Image())
                << SetRect(Rect::XYWH((320 - 30 - nextTopUp_->textWidth()) / 2, 170, 32, 32))
                << SetBitmap(assets::icons_24::money_bag);
            with(nextTopUp_)
                << RightOf(topUp_);
        }

        ~PiggyBank() override {
            saveState();
        }

    protected:

        struct State {
            int32_t value = 0;
            int32_t allowance = 0;
            TinyDate lastTopUp;

            void load(ini::Reader & reader) {
                reader
                    >> ini::Section("piggybank")
                        >> ini::Field("value", value)
                        >> ini::Field("allowance", allowance)
                        >> ini::Field("lastTopUp", lastTopUp);
            }

            void save(ini::Writer & writer) {
                writer 
                    << ini::Section("piggybank")
                        << ini::Field("value", value)
                        << ini::Field("allowance", allowance)
                        << ini::Field("lastTopUp", lastTopUp);
            }
        }; 

        void checkTopUp() {
            TinyDate now = time::now().date;
            TinyDate topUp{1, static_cast<uint8_t>((now.month() == 12) ? 1 : (now.month() + 1)), now.year()};
            daysTillTopUp_ = now.daysTillNextAnnual(topUp);
            int32_t changed = 0;
            if (state_.allowance > 0 && now.year() >= 2026) {
                while (state_.lastTopUp.year() < now.year() || state_.lastTopUp.month() < now.month()) {
                    state_.value += state_.allowance;
                    changed += state_.allowance;
                    state_.lastTopUp.incMonth();
                } 
            }
            if (changed) {
                state_.lastTopUp = now;
                InfoDialog::success("Top Up", STR(state_.allowance << " has been added to your piggy bank"));
                // save the state immediately - it's money:)
                saveState();
            }
        }

        void saveState() {
            auto f = fs::writeFile("state.ini");
            if (f != nullptr) {
                ini::Writer writer{*f};
                state_.save(writer);
            }
        }

        void onLoopStart() override {
            root_.flyIn();
        }

        void loop() override {
            using namespace ui;
            App<void>::loop();
            if (btnPressed(Btn::B) || btnPressed(Btn::Down)) {
                root_.flyOut();
                waitUntilIdle();
                exit();
            }
        }

    private:
        State state_;
        int32_t daysTillTopUp_ = 0;

        ui::Image * icon_;
        ui::Image * topUp_;
        ui::Label * ballance_;
        ui::Label * nextTopUp_;

    }; // rckid::Steps
    

} // namespace rckid