#include "Messages.h"

namespace rckid {

    void Messages::Chat::markRead() {
        if (unread_) {
            unread_ = false;
            Task::getOrCreate()->saveChats();
        }
    }

    void Messages::Chat::appendMessage(uint64_t from, String text) {
        LOG(LL_INFO, "New message from " << from << " in chat " << id_ << ": " << text);
        // open the conversation file for appending, create the message entry and then serialize
        auto f = fs::fileAppend(conversationPath());
        Entry msgEntry{Entry::Message(from, std::move(text))};
        serialize(f, msgEntry);
        f.close();
        // now either mark as unread, or if the chat is opened, append the entry to the chat view as well
        if (conversation_ != nullptr) {
            unread_ = conversation_->appendMessage(std::move(msgEntry));
        } else if (unread_ == false) {
            unread_ = true;
            // and save the chats so that unread status is preserved
            Task::getOrCreate()->saveChats();
        }
    }

} // namespace rckid