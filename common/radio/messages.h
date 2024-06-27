#pragma once

#include "definitions.h"

namespace rckid::radio::msg {

    enum class Id : uint8_t {
#define MESSAGE(NAME, ...) NAME,
#include "messages.inc.h"
    }; // msg::Id


    inline bool requiresAck(Id id) {
        switch (id) {
#define MESSAGE(NAME, ACK_REQUIRED, ...) case Id::NAME: return ACK_REQUIRED;
#include "messages.inc.h"
            default:
                UNREACHABLE;
        }
    }

#define MESSAGE(NAME, ACK_REQUIRED, ...)                 \
    class NAME {                                                 \
    public:                                                      \
        static Id constexpr ID = Id::NAME;                     \
        Id const id = Id::NAME;                                \
        static NAME const & fromBuffer(uint8_t const * buffer) { \
            return * reinterpret_cast<NAME const *>(buffer);     \
        }                                                        \
        __VA_ARGS__                                              \
    } __attribute__((packed));                                   \
    static_assert(sizeof(NAME) <= 32);              

#include "messages.inc.h"



} // namespace rkid::radio::msg