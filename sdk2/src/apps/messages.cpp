#include <rckid/apps/messages.h>

namespace rckid {

    void write(BinaryWriter & w, Messages::Conversation::Entry::Kind const & kind) {
        w << static_cast<uint8_t>(kind);
    }

    void read(BinaryReader & r, Messages::Conversation::Entry::Kind & into) {
        uint8_t value;
        r >> value;
        into = static_cast<Messages::Conversation::Entry::Kind>(value);
    }

    void write(BinaryWriter & w, Messages::Conversation::Entry const & msg) {
        uint32_t size = w.bytesWritten();
        w 
            << msg.kind
            << msg.time
            << msg.sender
            << msg.payload;
        size = w.bytesWritten() - size + sizeof(uint32_t);
        w << size;
    }

    void read(BinaryReader & r, Messages::Conversation::Entry & into) {
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

}