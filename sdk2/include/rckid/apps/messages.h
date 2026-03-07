#pragma once

#include <rckid/ui/app.h>
#include <rckid/capabilities/wifi.h>
#include <rckid/apps/launcher.h>
#include <rckid/apps/dialogs/info_dialog.h>

namespace rckid {

    class Messages : public ui::App<void> {
    public: 

        String name() const override { return "Messages"; }

        Messages() {
            using namespace ui;
            carousel_ = addChild(new Launcher::BorrowedCarousel());
            WiFi * wifi = WiFi::instance();
            wifi->enable();
        }

        class Chat {
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

            }; // Messages::Chat::Entry

            Chat(ini::Reader & reader);

            String const & name() const { return name_; }

            ImageSource const & icon() const { return icon_; }

            int64_t id() const { return id_; }

            bool unread() const { return unread_; }

        protected:

            String name_;
            int64_t id_;
            ImageSource icon_;
            bool unread_ = false;

        }; // Messages::Chat

    protected:


        class ChatRoom : public ui::App<void> {

        }; // Messages::ChatRoom

        void onLoopStart() override;

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
        std::vector<unique_ptr<Chat>> chats_;



    }; // rckid::Messages

} // namespace rckid