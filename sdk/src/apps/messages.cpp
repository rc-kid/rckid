#include <rckid/ini.h>
#include <rckid/apps/messages.h>

#include <assets/icons_24.h>

namespace rckid {

    void write(BinaryWriter & w, Messages::Chat::Entry::Kind const & kind) {
        w << static_cast<uint32_t>(kind);
    }

    void read(BinaryReader & r, Messages::Chat::Entry::Kind & into) {
        uint32_t value;
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

    uint32_t Messages::Chat::readPrev(uint32_t offset, std::function<void(Entry)> cb, uint32_t n) {
        UNIMPLEMENTED;
    }

    uint32_t Messages::Chat::readNext(uint32_t offset, std::function<void(Entry)> cb, uint32_t n) {
        auto f = openRead();
        if (f == nullptr)
            return offset;
        Entry e;
        while (n-- > 0 && ! f->eof()) {
            f->binaryReader() >> e;
            cb(e);
        }
        return f->tell();
    }

    uint32_t Messages::Chat::append(Entry e) {
        // TODO should we mark as unread ?
        auto f = openWrite();
        ASSERT(f != nullptr);
        f->binaryWriter() << e;
        return f->tell();
    }

    unique_ptr<RandomReadStream> Messages::Chat::openRead() {
        return fs::readFile(fs::join("/apps/Messages/chats", STR(id_ << ".dat")));
    }

    unique_ptr<RandomWriteStream> Messages::Chat::openWrite() {
        return fs::writeFile(fs::join("/apps/Messages/chats", STR(id_ << ".dat")));
    }

    uint32_t Messages::Chat::seekPrev(uint32_t n, RandomReadStream * f) {
        UNIMPLEMENTED;
    }

    uint32_t Messages::Chat::seekNext(uint32_t n, RandomReadStream * f) {
        Entry e;
        while (n-- > 0 && ! f->eof())
            f->binaryReader() >> e;
        return f->tell();
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
                                App::run<ChatRoom>(c);
                            }}.withDecorator([chat = chat.get()](ui::MenuItem &, ui::Image * img, ui::Label *) {
                                if (chat->unread()) {
                                    img->addChild(new ui::Image{})
                                        << SetRect(Rect::XYWH(0, 0, 24, 24))
                                        << SetBitmap(assets::icons_24::exchange);
                                }
                            });
                    uint32_t offset = 0;
                    offset = chat->readNext(offset, [](Chat::Entry e) { 
                        LOG(LL_INFO, e.payload);
                    });
                    LOG(LL_INFO, "Final offset: " << offset);
                    chats_.push_back(std::move(chat));
                });
                return menu;
            });
    }

}