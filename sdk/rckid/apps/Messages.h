#pragma once

#include "../app.h"
#include "../task.h"
#include "../ui/form.h"
#include "../ui/label.h"
#include "../ui/image.h"
#include "../assets/fonts/OpenDyslexic128.h"
#include "../assets/fonts/OpenDyslexic64.h"
#include "../assets/icons_24.h"
#include "../wifi.h"
#include "../utils/ini.h"
#include "../utils/json.h"


namespace rckid {

    class Messages : public ui::Form<void> {
    public:

        class Task;
        class Conversation;

        /** Single chat room.  

            Each chat corresponds to a telegram chat, or group chat. Chats cannot be created by the user directly, but only joined via bot commands from the specified parent.

            Each chat is stored as a single file where messages are stored as entries one by one. Each message entry ends with the total size of the entry, which allows for easy backwards reading. 
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

                /** Creates an empty entry, should only be useful for deserialization purposes - otherwise entries should be created via the named static method constructors. 
                 */
                Entry(): kind{Kind::Text}, sender{0} { }

                static Entry Message(int64_t from, String text) {
                    return Entry{Kind::Text, timeNow(), from, std::move(text)};
                }

                friend void serialize(WriteStream & stream, Entry const & entry) {
                    serialize(stream, static_cast<uint32_t>(entry.kind));
                    serialize(stream, entry.time);
                    serialize(stream, entry.sender);
                    uint32_t size;
                    switch (entry.kind) {
                        case Kind::Text:
                            size = entry.payload.size();
                            // the first size will be serialized as part of the payload string
                            serialize(stream, entry.payload);
                            break;
                        default:
                            UNREACHABLE;
                    }
                    serialize(stream, size + HEADER_SIZE + 4);
                }

                friend void deserialize(ReadStream & stream, Entry & entry) {
                    uint32_t kind;
                    deserialize(stream, kind);
                    entry.kind = static_cast<Kind>(kind);
                    deserialize(stream, entry.time);
                    deserialize(stream, entry.sender);
                    switch (entry.kind) {
                        case Kind::Text:
                            deserialize(stream, entry.payload);
                            break;
                        default:
                            UNREACHABLE;
                    }
                    // total size, can be ignored, only useful for backwards searching
                    uint32_t size;
                    deserialize(stream, size); 
                }

            private:

                static constexpr uint32_t HEADER_SIZE = sizeof(uint32_t) + sizeof(TinyDateTime) + sizeof(int64_t) + sizeof(uint32_t);

                Entry(Kind kind, TinyDateTime time, int64_t sender, String payload):
                    kind{kind}, time{time}, sender{sender}, payload{std::move(payload)} {
                }
            }; // Messages::Chat::Entry

            /** Simple conversation files reader.
             
                Allows reading and seeking of individual message entries. Since the FatFS only allows single handle per file, the reader should must be used carefully and constructed only when needed, destroyed immediately afterwards.
             */
            class Reader {
            public:

                /** Creates the conversation reader, opens the conversation file and seeks to the beginning of the conversation.
                 */
                Reader(Chat const & chat):
                    f_{fs::fileRead(chat.conversationPath())} {
                }

                /** Creates the conversation reader, opens the conversation file and seeks to the given position (in bytes)
                 */
                Reader(Chat const & chat, uint32_t position):
                    f_{fs::fileRead(chat.conversationPath())} {
                    f_.seek(position);
                }

                /** Returns true if the reader is at the end of the conversation file.
                 */
                bool eof() const { return f_.eof(); }

                /** Seeks given number of entries from the current position.
                 
                    Positive values seek to newer messages, negative values seek to older messages. Returns the number of entries actually seeked, which may be less than requested if start or end of the file is reached.
                 */
                int32_t seek(int32_t offset) {
                    int32_t count = 0;
                    while (offset < 0) {
                        uint32_t curr = f_.tell();
                        if (curr == 0)
                            break;
                        f_.seek(curr - 4);
                        uint32_t previousSize;
                        deserialize(f_, previousSize);
                        f_.seek(curr - previousSize);
                        count -= 1;
                        ++offset;
                    }
                    while (offset > 0) {
                        if (f_.eof())
                            break;
                        Chat::Entry e;
                        deserialize(f_, e);
                        count += 1;
                        --offset;
                    }
                    return count;
                }

                /** Seeks to the start of the conversation. 
                 */
                void seekStart(int32_t offset = 0) {
                    f_.seek(0);
                    if (offset != 0)
                        seek(offset);
                }

                /** Seeks to the end of the conversation file.
                 */
                void seekEnd(int32_t offset = 0) {
                    f_.seek(f_.size());
                    if (offset != 0)
                        seek(offset);
                }

                /** Reads up to maxEntries from current position.
                 
                    Calls the provided callback for each entry read. Returns the number of entries actually read.
                 */
                uint32_t read(uint32_t maxEntries, std::function<void(Entry)> callback) {
                    uint32_t count = 0;
                    while (count < maxEntries && ! eof()) {
                        Chat::Entry e;
                        deserialize(f_, e);
                        callback(std::move(e));
                        ++count;
                    }
                    return count;
                }

                uint32_t currentPos() const { return f_.tell(); }

            private:

                fs::FileRead f_;

            }; // Messages::Chat::Reader

            /** Telegram id of the chat. 
             */
            int64_t id() const { return id_; }

            /** Name of the chat, used by RCKid, changeable by the user. 
             */
            String const & name() const { return name_; }

            /** Chat icon to be used in RCKid, changeable by the user. 
             */
            Icon const & image() const { return image_; }

            /** True if there are unread messages in the chat. 
             */
            bool unread() const { return unread_; }

        private:
            friend class Task;
            friend class Conversation;
            
            Chat(int64_t id, String const & name):
                id_{id}, name_{name} {
            }

            Chat(ini::Reader & reader) {
                while (std::optional<std::pair<String, String>> kv = reader.nextValue()) {
                    if (kv->first == "id") {
                        id_ = std::atoll(kv->second);
                    } else if (kv->first == "name") {
                        name_ = kv->second;
                    } else if (kv->first == "image") {
                        image_ = kv->second;
                    } else if (kv->first == "unread") {
                        unread_ = (kv->second == "true");
                    } else {
                        LOG(LL_ERROR, "Invalid chat field " << kv->first);
                    }
                }
            }

            void saveTo(ini::Writer & writer) const {
                writer.writeSection("chat");
                writer.writeValue("id", STR(id_));
                writer.writeValue("name", name_);
                if (image_.isFile())
                    writer.writeValue("image", image_.filename());
                if (unread_)
                    writer.writeValue("unread", "true");
            }

            void appendMessage(uint64_t from, String text) {
                LOG(LL_INFO, "New message from " << from << " in chat " << id_ << ": " << text);
                // open the conversation file for appending, create the message entry and then serialize
                auto f = fs::fileAppend(conversationPath());
                Entry msgEntry{Entry::Message(from, std::move(text))};
                serialize(f, msgEntry);
                f.close();
                // now either mark as unread, or if the chat is opened, append the entry to the chat view as well
                if (conversation_ != nullptr) {
                    unread_ = conversation_->appendMessage(std::move(msgEntry));
                } else {
                    unread_ = true;
                    // TODO do we want to save this somewhere?
                }
            }

            String conversationPath() const {
                return STR("/apps/Messages/chats/" << id_ << ".dat");
            }

            int64_t id_;
            String name_;
            Icon image_{assets::icons_64::chat};
            bool unread_ = false;

            Conversation * conversation_ = nullptr;

        }; // Messages::Chat

        /** Single conversation.
         
            The conversation is a simple scrollable view of the conversation history that displays the messages and buttons for viewing the extra content (images, audio). 
         
         */
        class Conversation : public ui::Form<void> {
        public:

            /** Single messgae visualization.
                
                TODO in the future, this can be a lot more, but for now make it simple, a small label for who? 
             */
            class Message : public ui::Widget {
            public:
                
                Message(Chat::Entry && entry, Contact const * sender):
                    ui::Widget{Rect::WH(320, 40)},
                    entry_{std::move(entry)},
                    sender_{sender}
                {
                    who_ = addChild(new ui::Label{Rect::XYWH(4, 4, 312, 16), sender_ != nullptr ? sender_->name : "Me"});
                    what_ = addChild(new ui::Label{Rect::XYWH(4, 16, 312, 24), entry_.payload});
                    who_->setFont(Font::fromROM<assets::Iosevka16>());
                    what_->setFont(Font::fromROM<assets::Iosevka24>());
                    if (sender_ != nullptr) {
                        who_->setColor(sender_->color);
                        what_->setColor(sender_->color);
                        who_->setHAlign(HAlign::Left);
                        what_->setHAlign(HAlign::Left);
                    } else {
                        who_->setHAlign(HAlign::Right);
                        what_->setHAlign(HAlign::Right);
                    }
                }

                bool isOwnMessage() const { return sender_ == nullptr; }

            private:
                Chat::Entry entry_;
                // who sent the message, nullptr if own msg
                Contact const * sender_;
                ui::Label * who_;
                ui::Label * what_;
            }; // Conversation::Message

            /** Use umbrella names for all messages stuff.
             */
            String name() const override { return "Messages"; }

            String title() const override { return chat_->name(); }

            Conversation(Chat * chat):
                ui::Form<void>{Rect::XYWH(0, 0, 320, 240)},
                chat_{chat}
            {
                chat_->conversation_ = this;
                // as we always display newest messages, mark the chat as read
                chat_->unread_ = false;
                view_ = g_.addChild(new ui::ScrollView{Rect::XYWH(0, 24, 320, 216)});
                // load contacts as we will need them
                Contact::forEach([this](Contact c){
                    if (c.telegramId != 0)
                        contacts_.insert(std::make_pair(c.telegramId, std::move(c)));
                });
                // load the messages
                loadMessages();
                view_->scrollBottomLeft();
            }

            ~Conversation() override {
                chat_->conversation_ = nullptr;
            }

        protected:

            friend class Chat;

            void update() override {
                ui::Form<void>::update();
                // quit the conversation view when B is pressed
                if (btnPressed(Btn::B))
                    exit();
                // A sends new message
                if (btnPressed(Btn::A)) {
                    auto text = App::run<TextDialog>("Enter message");
                    if (text.has_value()) {
                        Messages::Task::getOrCreate()->sendMessage(
                            chat_->id(),
                            text.value()
                        );
                    }
                }
            }

            void loadMessages() {
                Chat::Reader reader{*chat_};
                reader.read(20, [this](Chat::Entry e){
                    LOG(LL_INFO, "Loaded message: " << e.payload);
                    addMessage(std::move(e));
                });
            }

            bool atEnd() const {
                // TODO does not work
                return true;
            }

            /** Appends message to the conversation. 
             * 
             */
            bool appendMessage(Chat::Entry && entry) {
                if (! atEnd())
                    return true;
                addMessage(std::move(entry));
                view_->scrollBottomLeft();
                return false;
            }

            void addMessage(Chat::Entry && entry) {
                ui::Widget * last = view_->lastChild();
                Coord offset = last == nullptr ? 0 : last->y() + last->height();
                Contact * sender = getContactFor(entry.sender);
                Message * msg = new Message(std::move(entry), sender);
                msg->setY(offset);
                view_->addChild(msg);
            }

            Contact * getContactFor(int64_t sender) {
                // check if the sender is us
                if (sender == Messages::Task::getOrCreate()->botId_)
                    return nullptr;
                // check if we have the sender
                auto it = contacts_.find(sender);
                if (it != contacts_.end())
                    return &it->second;
                return & unknown_;
            }

        private:
            Chat * chat_;

            ui::ScrollView * view_;
            std::unordered_map<int64_t, Contact> contacts_;
            Contact unknown_{"Unknown"};
        }; // Conversation

        /** Task responsible for the message delivery and actions. 
         */
        class Task : public rckid::Task {
        public:

            static Task * getOrCreate() {
                if (instance_ == nullptr)
                    instance_ = new Task{};
                return instance_;
            }

            ~Task() override {
                wifi_->enable(false);
                for (Chat * chat : chats_)
                    delete chat;
            }

            Chat * getKnownChat(uint64_t chatId) {
                auto it = chatMap_.find(chatId);
                if (it != chatMap_.end())
                    return it->second;
                return nullptr;
            }

        protected:

            void tick() override {
                uint64_t now = uptimeUs64();
                if (wifi_->connected() && (now >= nextUpdateTime_)) {
                    LOG(LL_INFO, "Checking for new updates...");
                    wifi_->https_get(
                        "api.telegram.org", 
                        STR("/bot" << botId_ << ':' << botToken_ << "/getUpdates?offset=" << lastOffset_),
                        [this](uint32_t status, uint32_t size, uint8_t const * data) {
                            if (status == 200) {
                                retries_ = 0;
                                processUpdate(size, data);
                            } else {
                                LOG(LL_ERROR, "Failed to get updates, status: " << status);
                                // if wifi is connected still, schedule a retry with some backoff
                                // immediately 0.5s 1s 2s 4s 8s 16s 32s...
                                if (wifi_->connected() && retries_ < 5) {
                                    nextUpdateTime_ = uptimeUs64() + (retries_ == 0 ) ? 0 : (1 << (retries_ -1)) * 500000;
                                    if (++retries_ > 8)
                                        retries_ = 0;                                     
                                }
                            }
                        }
                    );
                    nextUpdateTime_ = now + 60 * 1000000; // check every minute
                }
                // and see if there is anything to send
                processOutgoingMessages();
            }

            void requestUpdate() {
                nextUpdateTime_ = 0;
            }

            void processUpdate(uint32_t size, uint8_t const * data) {
                auto s = MemoryReadStream{data, size};
                json::Object res = json::parse(s);
                //LOG(LL_INFO, "Update response: \n" << res);
                if (res["ok"].asBoolean()) {
                    bool changed = false;
                    bool newMsg = false;
                    for (auto & item : res["result"]) {
                        int64_t updateId = item["update_id"].asInteger();
                        // if we have already seen the message, skip it
                        if (lastOffset_ > updateId)
                            continue;
                        if (item.has("message")) {
                            auto & msg = item["message"];
                            int64_t from = msg["from"]["id"].asInteger();
                            int64_t chatId = msg["chat"]["id"].asInteger();
                            String text = msg["text"].asString();
                            Chat * chat = getKnownChat(chatId);
                            if (text.startsWith("/")) {
                                if (from != parentId_)
                                    LOG(LL_ERROR, "Ignoring command from unknown user " << from << ": " << text);
                                else
                                    processCommand(std::move(text), chatId);
                            } else if (chat == nullptr) {
                                LOG(LL_ERROR, "Message from " << from << " in unknown chat " << chatId << ": " << text);
                            } else {
                                chat->appendMessage(from, text);
                                if (!newMsg) {
                                    newMsg = true;
                                    rumblerEffect(RumblerEffect::OK());
                                }
                            }
                        }
                        lastOffset_ = updateId + 1;
                        changed = true;
                    }
                    if (changed)
                        requestUpdate();
                }
            }

            void processCommand(String command, int64_t chatId) {
                LOG(LL_INFO, "Processing command: " << command << " (in chat " << chatId << ")");
                // telegram global message - should introduce the bot
                if (command.startsWith("/start")) {
                    // TODO
                // telegram global message - should print help
                } else if (command.startsWith("/help")) {
                    // TODO
                // rckid command - join new chat, which creates the chat structure and allows messages in that chat to be processed
                } else if (command.startsWith("/join ")) {
                    if (chatMap_.find(chatId) != chatMap_.end()) {
                        LOG(LL_ERROR, "Chat " << chatId << " already joined");
                        sendMessage(chatId, STR("RCKID: Chat already joined."), false);
                        return;
                    }
                    String chatName = command.substr(6);
                    Chat * chat = new Chat{chatId, chatName};
                    chats_.push_back(chat);
                    chatMap_.insert(std::make_pair(chat->id_, chat));
                    saveChats();
                    //c_->setItem(chats_.size() - 1, Direction::Up);
                    sendMessage(chatId, STR("RCKID: Chat '" << chatName << "' enabled."), false);
                } else {
                    LOG(LL_ERROR, "Unknown command: " << command);
                    sendMessage(chatId, STR("RCKID: Unknown command: " << command << ", use /help for list of available commands."), false);
                }
            }

            void sendMessage(int64_t chatId, String text, bool addToChat = true) {
                outgoing_ = new OutgoingMessage{chatId, std::move(text), addToChat, outgoing_};
            }

            void processOutgoingMessages() {
                if (outgoing_ == nullptr)
                    return;
                // remove the message from the outgoing queue
                OutgoingMessage * msg = outgoing_;
                outgoing_ = outgoing_->next;
                // if we are not connected, there is nothing we can do
                if (! wifi_->connected()) {
                    if (msg->addToChat)
                        InfoDialog::error("No WiFi", "Cannot send message, no wifi connection available");
                    else 
                        LOG(LL_ERROR, "Cannot send message, wifi not connected");
                    delete msg;
                } else {
                    wifi_->https_get(
                        "api.telegram.org", 
                        STR("/bot" << botId_ << ':' << botToken_ << "/sendMessage?chat_id=" << msg->chatId << "&text=" << urlEncode(msg->text.c_str())),
                        [this, msg](uint32_t status, [[maybe_unused]] uint32_t size, [[maybe_unused]] uint8_t const * data) {
                            if (status == 200) {
                                LOG(LL_INFO, "Message sent successfully");
                                if (msg->addToChat) {
                                    Chat * chat = getKnownChat(msg->chatId);
                                    if (chat != nullptr)
                                        chat->appendMessage(botId_, msg->text);
                                }
                                delete msg;
                            } else {
                                if (++(msg->retries) > 10) {
                                    if (msg->addToChat)
                                        InfoDialog::error("WiFi error", "Cannot send message, too many retries");
                                    else 
                                        LOG(LL_ERROR, "Cannot send message, too many retries");
                                    delete msg;
                                } else {
                                    msg->next = outgoing_;
                                    outgoing_ = msg;
                                }
                            }
                        }
                    );
                }
            }

            /** Home folder of the task is shared with the messages app.
             */
            String homeFolder() const { return "/apps/Messages"; }

        private:

            friend class Messages;

            struct OutgoingMessage {
                int64_t chatId;
                String text;
                uint32_t retries = 0;
                bool addToChat;
                OutgoingMessage * next;

                OutgoingMessage(int64_t chatId, String && text, bool addToChat, OutgoingMessage * next):
                    chatId{chatId}, text(std::move(text)), addToChat(addToChat), next(next) {
                }
            };

            Task():
                wifi_{WiFi::getOrCreateInstance()} 
            {
                fs::createFolders(fs::join(homeFolder(), "chats"));
                wifi_->enable();
                wifi_->connect();
                loadSettings();
                loadChats();
            }

            /** Loads the messenger settings. 
             */
            void loadSettings() {
                ini::Reader ini{fs::fileRead(fs::join(homeFolder(), "settings.ini"))};
                if (! ini.eof()) {
                    while (auto section = ini.nextSection()) {
                        if (section.value() == "myself") {
                            while (auto kv = ini.nextValue()) {
                                if (kv->first == "token") {
                                    botToken_ = kv->second;
                                } else if (kv->first == "id") {
                                    botId_ = std::atoll(kv->second);
                                } else {
                                    LOG(LL_ERROR, "Unknown telegram setting: " << kv->first);
                                }
                            }
                        } else if (section.value() == "parent") {
                            while (auto kv = ini.nextValue()) {
                                if (kv->first == "id") {
                                    parentId_ = std::atoll(kv->second);
                                } else {
                                    LOG(LL_ERROR, "Unknown telegram setting: " << kv->first);
                                }
                            }
                        } else {
                            LOG(LL_ERROR, "Invalid settings section: " << section.value());
                        }
                    }
                }
            }

            /** Loads the chats. 
             */
            void loadChats() {
                ini::Reader ini{fs::fileRead(fs::join(homeFolder(), "chats.ini"))};
                if (! ini.eof()) {
                    while (auto section = ini.nextSection()) {
                        if (section.has_value() && section.value() == "chat") {
                            Chat * chat = new Chat{ini};
                            chats_.push_back(chat);
                            chatMap_.insert(std::make_pair(chat->id_, chat));
                        }
                    }
                }
            }

            void saveChats() {
                fs::createFolders(homeFolder());
                ini::Writer writer{fs::fileWrite(fs::join(homeFolder(), "chats.ini"))};
                for (auto chat : chats_)
                    chat->saveTo(writer);
            }
            

            WiFi * wifi_;
            int64_t botId_;
            String botToken_;
            int64_t parentId_;

            int64_t lastOffset_ = 0;
            uint64_t nextUpdateTime_ = 0;
            uint32_t retries_ = 0;

            std::vector<Chat *> chats_;
            std::unordered_map<uint64_t, Chat *> chatMap_;

            OutgoingMessage * outgoing_ = nullptr;

            static inline Task * instance_ = nullptr;

        }; // Messages::Task

        String name() const override { return "Messages"; }

        Messages():
            ui::Form<void>{},
            t_{Task::getOrCreate()}
        {

            c_ = g_.addChild(new ui::EventBasedCarousel{
                [this](){ return t_->chats_.size(); },
                [this](uint32_t index, Direction direction) {
                    Chat * c = t_->chats_[index];
                    c_->set(c->name(), c->image(), direction);
                }
            });
            c_->setRect(Rect::XYWH(0, 160, 320, 80));
            c_->setFont(Font::fromROM<assets::OpenDyslexic64>());
            c_->focus();
            if (t_->chats_.size() > 0)
                c_->setItem(0, Direction::Up);
            else
                c_->showEmpty(Direction::Up);
        }

        ~Messages() override {
        }

    protected:
    
        void update() override {
            ui::Form<void>::update();
            if ((btnPressed(Btn::A) || btnPressed(Btn::Up)) && t_->chats_.size() > 0) {
                uint32_t index = c_->currentIndex();
                if (index < t_->chats_.size()) {
                    Chat * chat = t_->chats_[index];
                    App::run<Conversation>(chat);
                }
            }
            if (btnPressed(Btn::B) || btnPressed(Btn::Down))
                exit();
        }

    private:
        
        Task * t_;

        ui::EventBasedCarousel * c_;

    }; // rckid::Messages

} // namespace rckid
