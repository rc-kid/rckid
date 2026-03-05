#pragma once

#include <rckid/ui/app.h>
#include <rckid/capabilities/wifi.h>
#include <rckid/apps/launcher.h>
#include <rckid/apps/dialogs/info_dialog.h>

namespace rckid {

    class Messages : public ui::App<void> {
    public: 

        String name() const override { return "Friends"; }

        Messages() {
            using namespace ui;
            carousel_ = addChild(new Launcher::BorrowedCarousel());
            WiFi * wifi = WiFi::instance();
            wifi->enable();
        }

        class Conversation {
        public:
            class Entry {
            public:
                enum class Kind {
                    Text,
                };
                Kind kind;
                TinyDateTime time;
                int64_t sender;
                String payload;

                friend void write(BinaryWriter &, Entry const &);
                friend void read(BinaryReader &, Entry &);

            }; // Messages::Conversation::Entry

        }; // Messages::Conversation

    protected:


        class ChatRoom : public ui::App<void> {

        }; // Messages::ChatRoom

        void onLoopStart() override {
            using namespace ui;
            with(carousel_) 
                << ResetMenu([]() { 
                    auto menu = std::make_unique<ui::Menu>();
                    // TODO add conversations
                    return menu;
                });
        }

        void onFocus() override {
            ui::App<void>::onFocus();
            focusWidget(carousel_);
        }

        void onBlur() override {
            ui::App<void>::onBlur();
        }

        void loop() {
            ui::App<void>::loop();
            if (btnPressed(Btn::B) || btnPressed(Btn::Down)) {
                ASSERT(carousel_->atRoot());
                exit();
            }

        }
 

    private:

        Launcher::BorrowedCarousel * carousel_;



    }; // rckid::Messages

} // namespace rckid