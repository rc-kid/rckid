#pragma once

#include "../rckid.h"

/** \section Messages
    \addtogroup Communication
 */

namespace rckid::msg {

    enum class Id : uint8_t {
        #define MESSAGE(ID, NAME, ...) NAME ID, 
        #include "messages.inc.h"
    }; 

    inline Id getIdFrom(uint8_t const * buffer) {
        if (*buffer < 0x80)
            return static_cast<Id>(*buffer & 0b11000000);
        else 
            return static_cast<Id>(*buffer);
    }


    inline bool requiresAck(Id id) {
        switch (id) {
            #define MESSAGE(ID, NAME, ACK_REQUIRED, ...) case Id::NAME: return ACK_REQUIRED;
            #include "messages.inc.h"
            default:
                UNREACHABLE;
        }
    }

#define MESSAGE(ID_HINT, NAME, ACK_REQUIRED, ...)                         \
    class NAME {                                                 \
    protected:  \
        uint8_t id_ = static_cast<uint8_t>(Id::NAME); \
    public:                                                      \
        static Id constexpr ID = Id::NAME;                       \
        static NAME const & fromBuffer(uint8_t const * buffer) { \
            return * reinterpret_cast<NAME const *>(buffer);     \
        }     \
        Id id() const { return getIdFrom(reinterpret_cast<uint8_t const*>(this)); }           \
        __VA_ARGS__                                              \
    } __attribute__((packed));                                   \
    static_assert(sizeof(NAME) <= 32);              

#include "messages.inc.h"

} // namespace rckid::msg
