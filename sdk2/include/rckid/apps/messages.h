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

        /** Single chat. 
         
            Chats have name, id of the corresponding telegram chat and an icon. A chat can also indicate that it has unread messages. The chat itself then comprises of chat entries, which are stored in a separate file for each chat. Those messages have format that allows seeking forwards as well as backwards so that partial scrolling can be implemented. 
         */
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

            /** Name of the chat (user visible)
             */
            String const & name() const { return name_; }

            /** Icon used for the chat user visible). 
             */
            ImageSource const & icon() const { return icon_; }

            /** Telegram ID of the chat. 
             */
            int64_t id() const { return id_; }

            /** True if there are any unread messages of the chat. 
             */
            bool unread() const { return unread_; }

        protected:

            String name_;
            int64_t id_;
            ImageSource icon_;
            bool unread_ = false;

        }; // Messages::Chat

    protected:

        /** Chat window. 
         
            A dedicated app that shows the conversation within a single chat. 
         */
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