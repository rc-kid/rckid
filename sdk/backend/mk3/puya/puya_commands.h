#pragma once

#include <rckid/device.h>
#include <platform/tinydate.h>
#include <config.h>

class KidDo;

namespace rckid {

    namespace mkIV {

    /** State of the device. 
     
        Contains information about the power state, button states, interrupts, accelerometer, voltages, etc. It is expected to be read every tick so tailored to preserve necessary information and often changing values. 
     */
    class DeviceState {
    public:
        enum class State : uint8_t {
            Off = 0,
            OffDC = 1,
            Wake = 2,
            Alarm = 3,
            PowerDown = 4,
            On = 7,
        };

        /** Returns the power state of the device.
         */
        State state() const { return static_cast<State>(device_ & STATE_MASK); }

        bool debugMode() const { return (device_ & DEBUG_MODE_MASK) != 0; }
        bool alarmInterrupt() const { return (device_ & ALARM_INTERRUPT_MASK) != 0; }
        bool dcPower() const { return (device_ & DC_POWER_MASK) != 0; }
        bool charging() const { return (device_ & CHARGING_MASK) != 0; }

        bool button(Btn b) const { return (buttons_ & (1 << static_cast<uint32_t>(b))) != 0; }

    protected:

        friend class ::KidDo;

        void setState(State s) { device_ = (device_ & ~STATE_MASK) | static_cast<uint8_t>(s); }

        void setDebugMode(bool enabled) {
            if (enabled)
                device_ |= DEBUG_MODE_MASK;
            else
                device_ &= ~DEBUG_MODE_MASK;
        }

        void setAlarmInterrupt(bool enabled) {
            if (enabled)
                device_ |= ALARM_INTERRUPT_MASK;
            else
                device_ &= ~ALARM_INTERRUPT_MASK;
        }

        void setDcPower(bool enabled) {
            if (enabled)
                device_ |= DC_POWER_MASK;
            else
                device_ &= ~DC_POWER_MASK;
        }

        void setCharging(bool enabled) {
            if (enabled)
                device_ |= CHARGING_MASK;
            else
                device_ &= ~CHARGING_MASK;
        }

        bool setButton(Btn b, bool pressed) {
            uint32_t btnId = static_cast<uint32_t>(b);
            if (button(b) == pressed)
                return false;
            pressed ? (buttons_ |= (1 << btnId)) : (buttons_ &= ~(1 << btnId));
            return true;
        }

    private:
        static constexpr uint8_t STATE_MASK           = 0x07;
        static constexpr uint8_t DEBUG_MODE_MASK      = 0x08;
        static constexpr uint8_t ALARM_INTERRUPT_MASK = 0x10;
        static constexpr uint8_t DC_POWER_MASK        = 0x20;
        static constexpr uint8_t CHARGING_MASK        = 0x40;

        uint8_t device_ = 0;
        uint8_t vcc_ = 0;

        // button state (button indices correspond to the Btn enum values)
        uint16_t buttons_ = 0;
        // accelerometer readings 8bit precision
        uint8_t accelX_ = 0;
        uint8_t accelY_ = 0;
        uint8_t accelZ_ = 0;

    } __attribute__((packed)); // rckid::DeviceState

    /** Extension of device state with less frequently required/updated information. 
     */
    class ExtendedState : public DeviceState {
    public:
        uint32_t const version = RCKID_AVR_FIRMWARE_VERSION;
        TinyDateTime time;

        uint16_t temp() const { return temp_; }

    protected:

    


        uint32_t uptime = 0;
        int16_t temp_ = 0;

    protected:
    } __attribute__((packed)); // rckid::ExtendedState 

    /** Transferrable state is the full device state held in the IO MCU RAM that can be transferred as a continuous block of data.
     
        This also includes the communication buffers.
     */
    class TransferrableState : public ExtendedState {
    public:
        bool debugUart = true;
        uint16_t bufferUsed = 0;
        uint8_t buffer[256];
    } __attribute__((packed)); // rckid::TransferrableState

    }   // namespace mkIV

} // namespace rckid

namespace rckid::cmd {
    #define COMMAND(MSG_ID, NAME, ...)                               \
        PACKED(class NAME  {                                         \
        public:                                                      \
            static uint8_t constexpr ID = MSG_ID;                    \
            uint8_t const id = MSG_ID;                               \
            static NAME const & fromBuffer(uint8_t const * buffer) { \
                return * reinterpret_cast<NAME const *>(buffer);     \
            }                                                        \
            __VA_ARGS__                                              \
        });                                                          \
        static_assert(sizeof(NAME) <= 131);              

    #include "puya_commands.inc"

    inline uint8_t getCommandSize(uint8_t id) {
    #define COMMAND(MSG_ID, NAME, ...)                               \
            case MSG_ID:                                             \
                return sizeof(NAME);                                 \

        switch (id) {
        #include "puya_commands.inc"
            default:
                return 0;
        }
    }

} // namespace rckid::cmd