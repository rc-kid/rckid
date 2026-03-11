#pragma once

#include <rckid/stream.h>
#include <rckid/ui/app.h>
#include <rckid/ui/chat_bubble.h>
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
            auto l = addChild(new ui::TextChatBubble())
                << SetRect(Rect::XYWH(5, 25, 300, 50))
                << SetBg(Color::RGB(255, 0, 0))
                << SetFg(Color::RGB(255, 255, 255))
                << SetText("Hello World\nIt's a me!");
//            l->fitToText();
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

            uint32_t readPrev(uint32_t offset, std::function<void(Entry)> cb, uint32_t n = 50);
            uint32_t readNext(uint32_t offset, std::function<void(Entry)> cb, uint32_t n = 50);

            uint32_t append(Entry e);

        protected:

            unique_ptr<RandomReadStream> openRead();
            unique_ptr<RandomWriteStream> openWrite();

            uint32_t seekPrev(uint32_t n, RandomReadStream * f);
            uint32_t seekNext(uint32_t n, RandomReadStream * f);

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
        public:



        protected:
            class TextMessage : public ui::ChatBubble {
            public:

            }; // ChatRoom::TextMessage

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