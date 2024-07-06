#pragma once

#include <vector>


#include "platform.h"
#include "common/errors.h"

namespace rckid::radio {

    /** DeviceID
     
        The device ID identifies an RCKid's device for the transmissions. The device ID is not tied to the device forever.
     */
    using DeviceId = uint8_t;

    /** Device ID 0 is reserved as broadcast device.
     */
    static constexpr DeviceId BroadcastId = 0;

    enum class ConnectionKind : uint8_t {

    }; // radio::ConnectionKind

    enum class BroadcastKind : uint8_t {

    }; // radio::BroadcastKind

    namespace msg {

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

#define MESSAGE(NAME, ACK_REQUIRED, ...)                         \
    class NAME {                                                 \
    public:                                                      \
        static Id constexpr ID = Id::NAME;                       \
        Id const id = Id::NAME;                                  \
        static NAME const & fromBuffer(uint8_t const * buffer) { \
            return * reinterpret_cast<NAME const *>(buffer);     \
        }                                                        \
        __VA_ARGS__                                              \
    } __attribute__((packed));                                   \
    static_assert(sizeof(NAME) <= 32);              

#include "messages.inc.h"

    } // radio::msg



    DeviceId id(); 

    /** Initializes the radio HW. 
     */
    void initialize(DeviceId deviceId);

    /** Enables the radio in receiver mode.
     
        When enabled, the radio will periodically transmit ping messages, unless the silent option is enabled. 
     */
    void enable(bool silent = false);

    /** Disables the radio. 
     
        Puts the associated hardware to sleep. 
     */
    void disable();


    void transmit(DeviceId target, uint8_t const * msg, size_t length);

    /** Sends given message. 
     */
    template<typename T>
    void sendMessage(DeviceId target, T const & msg) { 
        LOG("transmitting message");
        transmit(target, reinterpret_cast<uint8_t const *>(&msg), sizeof(T)); 
    }

    /** Polling interface for the radio.
     */
    void loop();


} // namespace rckid::radio