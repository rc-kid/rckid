#pragma once

#include <vector>


#include "platform.h"
#include "platform/utils/ring_buffer.h"
#include "rckid/errors.h"

namespace rckid::radio {

    /** DeviceID
     
        The device ID identifies an RCKid's device for the transmissions. The device ID is not tied to the device forever.
     */
    using DeviceId = uint8_t;

    /** Device ID 0 is reserved as broadcast device.
     */
    static constexpr DeviceId BroadcastId = 0;

    enum class BroadcastKind : uint8_t {

    }; // radio::BroadcastKind

    namespace msg {

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

    /** Transmits up to 32 bytes of packet data to given device id. 
     */
    void transmit(DeviceId target, uint8_t const * msg, size_t length);

    /** Sends given message. 
     */
    template<typename T>
    void sendMessage(DeviceId target, T const & msg) { 
        LOG("transmitting message");
        transmit(target, reinterpret_cast<uint8_t const *>(&msg), sizeof(T)); 
    }

    /** Polling interface for the radio.
     
        This needs to be called regularly to process the radio events. Since the implementation may use blocking API, it is generally not safe to call this from an ISR. 
     */
    void loop();

    #include "connection.inc.h"
    #include "controller.inc.h"

} // namespace rckid::radio