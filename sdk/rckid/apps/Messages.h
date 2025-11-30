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

            private:

                static constexpr uint32_t HEADER_SIZE = sizeof(uint32_t) + sizeof(TinyDateTime) + sizeof(int64_t) + sizeof(uint32_t);

                Entry(Kind kind, TinyDateTime time, int64_t sender, String payload):
                    kind{kind}, time{time}, sender{sender}, payload{std::move(payload)} {
                }
            }; // Messages::Chat::Entry

            /** Simple conversation files reader.
             
                Allows reading and seeking of individual message entries.
             */
            class Reader {

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
                serialize(f, Entry::Message(from, std::move(text)));
                f.close();
                // now either mark as unread, or if the chat is opened, append the entry to the chat view as well
                if (conversation_ != nullptr) {
                    // TODO tell the opened conversation and decide if unread or not
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
             */
            class Message : public ui::Widget {
            public:
                
                Message(Chat::Entry && entry, Contact const * sender):
                    ui::Widget{Rect::WH(320, 24)},
                    entry_{std::move(entry)},
                    sender_{sender}
                {
                }

                bool isOwnMessage() const { return sender_ == nullptr; }

            private:
                Chat::Entry entry_;
                // who sent the message, nullptr if own msg
                Contact const * sender_;
            }; // Conversation::Message

            /** Use umbrella names for all messages stuff.
             */
            String name() const override { return "Messages"; }

            String title() const override { return chat_->name(); }

            Conversation(Chat * chat):
                ui::Form<void>{Rect::XYWH(0, 0, 320, 240), /* raw */ true},
                chat_{chat},
                view_{Rect::XYWH(0, 24, 320, 216)}
            {
                chat_->conversation_ = this;
                // as we always display newest messages, mark the chat as read
                chat_->unread_ = false;
                g_.addChild(view_);
                // load contacts as we will need them
                Contact::forEach([this](Contact c){
                    if (c.telegramId != 0)
                        contacts_.insert(std::make_pair(c.telegramId, std::move(c)));
                });
                // TODO load the messages here
            }

            ~Conversation() override {
                chat_->conversation_ = nullptr;
            }
        protected:

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

        private:
            Chat * chat_;

            ui::ScrollView view_;
            std::vector<ui::Label> msgs_;
            std::unordered_map<int64_t, Contact> contacts_;
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
                if (wifi_->connected() && (uptimeUs64() >= nextUpdateTime_)) {
                    LOG(LL_INFO, "Checking for new updates...");
                    wifi_->https_get(
                        "api.telegram.org", 
                        STR("/bot" << botId_ << ':' << botToken_ << "/getUpdates?offset=" << lastOffset_),
                        [this](uint32_t status, uint32_t size, uint8_t const * data) {
                            if (status == 200)
                                processUpdate(size, data);
                        }
                    );
                    nextUpdateTime_ += 60 * 10000000; // check every minute
                }
            }

            void processUpdate(uint32_t size, uint8_t const * data) {
                auto s = MemoryReadStream{data, size};
                json::Object res = json::parse(s);
                //LOG(LL_INFO, "Update response: \n" << res);
                if (res["ok"].asBoolean()) {
                    for (auto & item : res["result"]) {
                        uint64_t updateId = item["update_id"].asInteger();
                        if (item.has("message")) {
                            auto & msg = item["message"];
                            int64_t from = msg["from"]["id"].asInteger();
                            int64_t chatId = msg["chat"]["id"].asInteger();
                            String text = msg["text"].asString();
                            Chat * chat = getKnownChat(chatId);
                            if (text[0] == '/') {
                                if (from != parentId_)
                                    LOG(LL_ERROR, "Ignoring command from unknown user " << from << ": " << text);
                                else
                                    processCommand(std::move(text), chatId);
                            } else if (chat == nullptr) {
                                LOG(LL_ERROR, "Message from " << from << " in unknown chat " << chatId << ": " << text);
                            } else {
                                chat->appendMessage(from, text);
                            }
                        }
                        // TODO do updateId + 1 as the next updte id
                    }
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
                        sendMessage(chatId, STR("RCKID: Chat already joined."));
                        return;
                    }
                    String chatName = command.substr(6);
                    Chat * chat = new Chat{chatId, chatName};
                    chats_.push_back(chat);
                    chatMap_.insert(std::make_pair(chat->id_, chat));
                    saveChats();
                    //c_.setItem(chats_.size() - 1, Direction::Up);
                    sendMessage(chatId, STR("RCKID: Chat '" << chatName << "' enabled."));
                } else {
                    LOG(LL_ERROR, "Unknown command: " << command);
                    sendMessage(chatId, STR("RCKID: Unknown command: " << command << ", use /help for list of available commands."));
                }
            }

            void sendMessage(uint64_t chatId, String const & text) {
                // TODO keep the message so that we can resend if there is network failure or something
                wifi_->https_get(
                    "api.telegram.org", 
                    STR("/bot" << botId_ << ':' << botToken_ << "/sendMessage?chat_id=" << chatId << "&text=" << urlEncode(text.c_str())),
                    [this](uint32_t status, uint32_t size, uint8_t const * data) {
                        if (status == 200)
                            LOG(LL_INFO, "Message sent successfully");
                    }
                );
                Chat * chat = getKnownChat(chatId);
                if (chat != nullptr)
                    chat->appendMessage(botId_, text);
            }

            /** Home folder of the task is shared with the messages app.
             */
            String homeFolder() const { return "/apps/Messages"; }

        private:

            friend class Messages;

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

            std::vector<Chat *> chats_;
            std::unordered_map<uint64_t, Chat *> chatMap_;

            static inline Task * instance_ = nullptr;

        }; // Messages::Task

        String name() const override { return "Messages"; }

        Messages():
            ui::Form<void>{},
            t_{Task::getOrCreate()},
            c_{
                [this](){ return t_->chats_.size(); },
                [this](uint32_t index, Direction direction) {
                    Chat * c = t_->chats_[index];
                    c_.set(c->name(), c->image(), direction);
                }
            }
        {

            g_.addChild(c_);
            c_.setRect(Rect::XYWH(0, 160, 320, 80));
            c_.setFont(Font::fromROM<assets::OpenDyslexic64>());
            c_.focus();
            if (t_->chats_.size() > 0)
                c_.setItem(0, Direction::Up);
            else
                c_.showEmpty(Direction::Up);
        }

        ~Messages() override {
        }

    protected:
    
        void update() override {
            ui::Form<void>::update();
            if ((btnPressed(Btn::A) || btnPressed(Btn::Up)) && t_->chats_.size() > 0) {
                uint32_t index = c_.currentIndex();
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

        ui::EventBasedCarousel c_;

    }; // rckid::Messages

} // namespace rckid

#ifdef FOOBAR

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

#endif

