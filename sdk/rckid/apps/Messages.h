#pragma once

#include "../rckid.h"
#include "../utils/stream.h"
#include "../ui/scrollview.h"
#include "../ui/panel.h"

namespace rckid {

    /** Simple messaging app
     
        Using telegram bot API, the messaging app should look & feel like a very simple telegram client.

        Chat app is just a list of text with possible icons for images or sounds sent. The chats are read from a file that contains the text and links to stored documents. The format is such that it is easy to travel between them in both ways

     */
    class Messages {
    public:

        /** Record in the message history.
         
            Records are simple structures that store the record kind (text message, other content, etc.), date & time when the message was received or sent (incoming or outgoing), the size of the payload and the payload itself. To make the file in which records are stored walkable in both directions, after the payload, the size of the payload is repeated again for simple backward searching.
         
            For the text messages, the payload is the null terminated string itself. For content kinds (audio, image), the payload is the filename of the downloaded content. 
         */
        class Record {
        public:
            enum class Kind : uint8_t {
                Text,
                File,
            }; // Record::Kind
            Kind kind;
            TinyDateTime time;
            uint64_t sender;
            uint32_t size;
            uint8_t data[];

            friend void serialize(WriteStream & stream, Record const & record) {
                serialize(stream, static_cast<uint8_t>(record.kind));
                serialize(stream, record.time);
                serialize(stream, record.sender);
                serialize(stream, record.size);
                serialize(stream, record.data, record.size);
                serialize(stream, record.size);
            }

            friend void deserialize(ReadStream & stream, Record * & record) {
                if (stream.eof()) {
                    record = nullptr;
                    return;
                }
                Messages::Record::Kind kind = static_cast<Messages::Record::Kind>(deserialize<uint8_t>(stream));
                TinyDateTime time = deserialize<TinyDateTime>(stream);
                uint64_t sender = deserialize<uint64_t>(stream);
                uint32_t size = deserialize<uint32_t>(stream);
                // alloocate the necessary space
                record = (Record*) malloc(sizeof(Record) + size);
                record->kind = kind;
                record->time = time;
                record->sender = sender;
                record->size = size;
                deserialize(stream, record->data, size);
                uint32_t sizeAgain = deserialize<uint32_t>(stream);
                ASSERT(sizeAgain == size);
            }
        }; // Messages::Record


        /** Manages single conversation. 
         
            
         */
        class Conversation {
        public:

        private:
            

        }; // Messages::Conversation


    //protected:

        /** Single conversation.
         
            The conversation is a simple scrollable view of the conversation history that displays the messages and buttons for viewing the extra content (images, audio). 
         
         */
        class ConversationView : public ui::Form<void> {
        public:

            /** Use umbrella names for all messages stuff.
             */
            String name() const override { return "Messages"; }

            ConversationView() {
                view_ = g_.addChild(new ui::ScrollView{});
                view_->setRect(Rect::XYWH(0, 20, 320, 200));
                for (uint32_t i = 0; i < 50; ++i) {
                    ui::Label * msg = view_->addChild(new ui::Label());
                    msg->setRect(Rect::XYWH(0, i * 20, 320, 20));
                    msg->setText(STR("Me: This is msg #" << i));
                    msgs_.push_back(msg);
                }
                view_->setOffsetTop(10);
                InfoDialog::info("Hello", "On noez");
                /*
                p_ = view_->addChild(new ui::Panel{});
                p_->setRect(Rect::XYWH(-10,-10,20,20));
                p_->setBg(ColorRGB::Blue());
                */
            }

        protected:

            void update() override {
                if (btnPressed(Btn::Up))
                    view_->setOffsetTop(view_->offsetTop() - 10);
                if (btnPressed(Btn::Down))
                    view_->setOffsetTop(view_->offsetTop() + 10);
                if (btnPressed(Btn::Left))
                    view_->setOffsetLeft(view_->offsetLeft() - 10);
                if (btnPressed(Btn::Right))
                    view_->setOffsetLeft(view_->offsetLeft() + 10);

            }

        private:

            ui::ScrollView * view_;
            std::vector<ui::Label *> msgs_;

        }; // Messages::ConversationView

    }; // Messages

} // namespace rckid