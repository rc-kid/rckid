#include <rckid/ini.h>
#include <rckid/apps/messages.h>

#include <assets/icons_24.h>

namespace rckid {

    void write(BinaryWriter & w, Messages::Chat::Entry::Kind const & kind) {
        w << static_cast<uint8_t>(kind);
    }

    void read(BinaryReader & r, Messages::Chat::Entry::Kind & into) {
        uint8_t value;
        r >> value;
        into = static_cast<Messages::Chat::Entry::Kind>(value);
    }

    void write(BinaryWriter & w, Messages::Chat::Entry const & msg) {
        uint32_t size = w.bytesWritten();
        w 
            << msg.kind
            << msg.time
            << msg.sender
            << msg.payload;
        size = w.bytesWritten() - size + sizeof(uint32_t);
        w << size;
    }

    void read(BinaryReader & r, Messages::Chat::Entry & into) {
        uint32_t start = r.bytesRead();
        uint32_t expectedSize;
        r
            >> into.kind
            >> into.time
            >> into.sender
            >> into.payload
            >> expectedSize;
        ASSERT(expectedSize == r.bytesRead() - start);
    }

    Messages::Chat::Chat(ini::Reader & reader):
        icon_{assets::icons_64::poo}
    {
        reader 
            >> ini::Field("name", name_) 
            >> ini::Field("id", id_)
            >> ini::Field("unread", unread_)
            >> ini::Field("icon", icon_);
    }

    void Messages::onLoopStart() {
        using namespace ui;
        with(carousel_) 
            << ResetMenu([this]() { 
                auto menu = std::make_unique<ui::Menu>();
                auto f = readFile("chats.ini");
                if (f == nullptr)
                    return menu;
                ini::Reader reader{*f};
                reader >> ini::SectionArray("chat", [&, this](ini::Reader & r) {
                    auto chat = std::make_unique<Chat>(r);
                    (*menu)
                        << ui::MenuItem{chat->name(), chat->icon(), [this, c = chat.get()]() {
                            // TODO open the chat
                    }}.withDecorator([chat = chat.get()](ui::MenuItem & item, ui::Image * img, ui::Label * label) {
                        if (chat->unread()) {
                            img->addChild(new ui::Image{})
                                << SetRect(Rect::XYWH(0, 0, 24, 24))
                                << SetBitmap(assets::icons_24::exchange);
                        }
                    });
                    chats_.push_back(std::move(chat));
                });
                return menu;
            });
    }

}