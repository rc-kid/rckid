#pragma once

#include <rckid/ui/app.h>
#include <rckid/apps/launcher.h>

namespace rckid {

    class Messages : public ui::App<void> {
    public: 

        String name() const override { return "Friends"; }

        Messages() {
            using namespace ui;
            carousel_ = addChild(new Launcher::BorrowedCarousel());
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
        }

        void onFocus() override {
            ui::App<void>::onFocus();
            focusWidget(carousel_);
        }

        void onBlur() override {
            ui::App<void>::onBlur();
        }

        void loop() {

        }
 

    private:

        Launcher::BorrowedCarousel * carousel_;



    }; // rckid::Messages

} // namespace rckid