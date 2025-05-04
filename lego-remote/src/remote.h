#pragma once

#include "platform/platform.h"

/** \page Remote Control Protocol

    

 */
namespace remote {

    namespace channel {

        /** Channel types supported by the remote protocol. 
         */
        enum class Kind : uint8_t {
            None = 0,
            Device,
            Motor, 
            CustomIO, 
            ToneEffect, 
            RGBStrip, 
            RGBColor,
        }; // channel::Kind


        class Device {
        public:
            struct Control {
                bool active;
            } __attribute__((packed)); // Device::Control

            struct Feedback {
                bool active() const { return status_ & ACTIVE; }
                bool connectionLost() const { return status_ & CONN_LOST; }
                bool overcurrent() const { return status_ & OVERCURRENT; }
                bool undervoltage() const { return status_ & UNDERVOLTAGE; }

                void setActive(bool value = true) {
                    value ? status_ |= ACTIVE : status_ &= ~ACTIVE;
                }

                void setConnectionLost(bool value = true) {
                    value ? status_ |= CONN_LOST : status_ &= ~CONN_LOST;
                }

                void setOvercurrent(bool value = true) {
                    value ? status_ |= OVERCURRENT : status_ &= ~OVERCURRENT;
                }

                void setUndervoltage(bool value = true) {
                    value ? status_ |= UNDERVOLTAGE : status_ &= ~UNDERVOLTAGE;
                }

            private:
                static constexpr uint8_t ACTIVE = (1 << 0);
                static constexpr uint8_t CONN_LOST = (1 << 1);
                static constexpr uint8_t OVERCURRENT = (1 << 2);
                static constexpr uint8_t UNDERVOLTAGE = (1 << 3);
                uint8_t status_ = 0;
            } __attribute__((packed)); // Device::Feedback

            struct Config {} __attribute__((packed));

        }; // channel::Device

        class Motor {
        public:
            enum class Mode : uint8_t {
                Coast, 
                Brake, 
                CW, 
                CCW
            }; // MotorChannel::Mode

            struct Control {
                Mode mode;
                uint8_t speed;

                bool operator == (Control const & other) const {
                    return mode == other.mode && speed == other.speed;
                }

                bool operator != (Control const & other) const {
                    return mode != other.mode || speed != other.speed;
                }

                static Control Coast() { return Control{Mode::Coast, 0}; }
                static Control Brake() { return Control{Mode::Brake, 0}; }
                static Control CW(uint8_t speed) { return Control{Mode::CW, speed}; }
                static Control CCW(uint8_t speed) { return Control{Mode::CCW, speed}; }

            } __attribute__((packed));

            struct Feedback {
                bool overcurrent;
                uint8_t v;
                uint8_t i;
            } __attribute((__packed__));

            struct Config {
                uint8_t overcurrent;
            } __attribute__((packed));
        }; // channel::Motor

        class CustomIO {
        public:
            enum class Mode : uint8_t {
                DigitalIn = 0, 
                DigitalOut = 1, 
                AnalogIn = 2, // ADC
                PWM = 3, // PWM
                Servo = 4, // Servo pulse 
            }; 

            struct Control {
                uint8_t value;
            } __attribute__((packed)); 

            struct Feedback {
                uint8_t value;
            } __attribute((__packed__));

            /** The custom IO channel is configured by its mode of operation, and the min and max pulse width for servo control, specified in microseconds. By default, this corresponds to a 270 degree standard servo with neutral position at 1500uS. You may wish to experiment with other values for particular servos.  
            */
            struct Config {
                Mode mode = Mode::DigitalIn;
                uint16_t servoStart = 500; // 0.5ms
                uint16_t servoEnd = 2500; // 2.5ms
                bool pullup = false;

                static Config input() { Config c; return c; }
                static Config inputPullup() { Config c; c.pullup = true; return c; }
                static Config output() { Config c; c.mode = Mode::DigitalOut; return c; }

            } __attribute__((packed));
        }; // channel::CustomIO


        /** 
         
            Hi-low siren = 450/600Hz in 0.5 sec interval
            Wail siren = 600 - 1200Hz sweep, sweep takes ~3 seconds up, 3 seconds down

            wail siren = 600-1200Hz sweep
         */
        class ToneEffect {
        public:
            enum class Effect : uint8_t {
                Tone, 
                Wail, // freq -> freq2 -> freq
                Yelp, // freq -> freq2
                HiLow, // freq | freq2 | freq
                Pulse, // freq | pause ...
            }; // Effect

            struct Control {
                Effect effect;
                uint16_t freq;
                uint16_t freq2;
                uint16_t transition;

                static Control tone(uint16_t freq, uint16_t duration = 0) {
                    return Control{Effect::Tone, freq, 0, duration};
                }

                static Control wail(uint16_t fStart, uint16_t fEnd, uint16_t duration) {
                    return Control{Effect::Wail, fStart, fEnd, duration};
                }

                static Control yelp(uint16_t fStart, uint16_t fEnd, uint16_t duration) {
                    return Control{Effect::Yelp, fStart, fEnd, duration};
                }

                static Control hiLow(uint16_t fStart, uint16_t fEnd, uint16_t duration) {
                    return Control{Effect::HiLow, fStart, fEnd, duration};
                }

                static Control pulse(uint16_t f, uint16_t dTone, uint16_t dNoTone) {
                    return Control{Effect::Pulse, f, dTone, dNoTone};
                }

            } __attribute__((packed)); 

            struct Feedback {

            } __attribute((__packed__));

            /** Configuration is the channel */
            struct Config {
                uint8_t outputChannel;
            } __attribute__((packed));
        }; // channel::ToneEffect

        class RGBStrip {
        public:
            struct Control {

            } __attribute__((packed)); 
            struct Feedback {

            } __attribute((__packed__));

            struct Config {

            } __attribute__((packed));
            Control control;
            Config config;
        }; // channel::RGBStrip

        class RGBColor {
        public:
            struct Control {

            } __attribute__((packed)); 
            struct Feedback {

            } __attribute__((packed));
            struct Config {

            } __attribute__((packed));
        }; // channel::RGBColor


    } // namespace remote::channel

    /** 

    */

    /** Defines new message. 
     */
    #define MESSAGE(NAME, ...) \
        class NAME : public msg::MessageHelper<NAME> { \
        public: \
            static uint8_t constexpr ID = __COUNTER__ - COUNTER_OFFSET; \
            static NAME const & fromBuffer(uint8_t const * buffer) { \
                return * reinterpret_cast<NAME const *>(buffer); \
            } \
            __VA_ARGS__ \
        } __attribute__((packed))

    namespace msg {

        /** Default channel on which remote devices start listening. This is identical for all devices so that automatic discovery is possible. 
         */
        constexpr uint8_t DefaultChannel = 86;

        /** Default address that all devices start listening to so that automatic discovery is possible. 
         */
        constexpr char const * DefaultAddress = "RCKID";

        class Message {
        public:
        protected:
            static int constexpr COUNTER_OFFSET = __COUNTER__ + 1;
        } __attribute__((packed)); 

        template<typename T> class MessageHelper : public Message {
        public:
            uint8_t const id = T::ID;
        } __attribute__((packed));

        /** Empty message. 
         */
        MESSAGE(Nop);

        /** Requests a remote device to identify itself to the controller. Upon receiving the request, the device is required to send the DeviceInfo message to the specified controller.
         */
        MESSAGE(RequestDeviceInfo, 
            uint8_t controllerAddress[5];

            RequestDeviceInfo(char const * addr) {
                memcpy(controllerAddress, addr, 5); 
            }
        );

        /** Device information. Contains the number of channels and a null terminated string containing the device name (up to 15 chars). 
         */
        MESSAGE(DeviceInfo, 
            uint16_t deviceId;
            uint8_t numChannels;
            char name[];
            
            DeviceInfo(uint8_t numChannels, uint16_t deviceId): 
                deviceId{deviceId}, 
                numChannels{numChannels} {
                name[0] = 0; // null terminate the name
            }

            DeviceInfo(uint8_t numChannels, uint16_t deviceId, char const * name):
                deviceId{deviceId},
                numChannels{numChannels} {
                uint8_t l = strnlen(name, 15);
                memcpy(this->name, name, l);
                this->name[l] = 0;
            }
        );

        /** Resets the pairing status ofthe device. The device will reset to default channel and default name and be ready to accept requests from other controllers. 
         */
        MESSAGE(Reset, 
            uint8_t controllerAddress[5];
        );

        /** Requests the device to pair with the provided controller. As part of the process also sets the device name the controller will use from now on and provides the channel to tune to. After the pairing is complete the device will not respond to any RequestDeviceInfo messages from different controllers. When paired, the device will also process messages with greater ID than Pair. 
         
            To distinguish which device the pair command is send to as all unpaired devices operate on the same channel, the pair command contains also the device id and device name identifiers, which must match those returned by the DeviceInfo command. 
         */
        MESSAGE(Pair, 
            uint8_t controllerAddress[5];
            uint8_t deviceAddress[5];
            uint8_t channel;
            uint16_t deviceId; 
            char deviceName[];

            Pair(char const * controllerAddress, char const * deviceAddress, uint8_t channel, uint16_t deviceId, char const * deviceName):
                channel{channel}, 
                deviceId{deviceId} {
                memcpy(this->controllerAddress, controllerAddress, 5);
                memcpy(this->deviceAddress, deviceAddress, 5);
                uint8_t l = strnlen(deviceName, 15);
                memcpy(this->deviceName, deviceName, l);
                this->deviceName[l] = 0;
            }
        );

        /** Requests the device to send information about the channels it contains, returning the ChannelInfo message. Channel information will be returned for at most 30 consecutive channels starting from the specified one (inclusive). If the device does not have more than 30 channels, it may choose to ingore the argument and always return information for all channels. 
         */
        MESSAGE(GetChannelInfo, 
            uint8_t fromChannel = 0;
        );

        /** Returns the information about channel types in the device. Returns up to 30 channels starting from the channel specified. If there is fewer channels available on the device, the list is terminated by 0 (None)
         */
        MESSAGE(ChannelInfo, 
            uint8_t channel;
            channel::Kind info[];

            ChannelInfo(uint8_t fromChannel = 1): channel{fromChannel} {}
        );

        /** Requests channel configuration for the given channel.  
         */
        MESSAGE(GetChannelConfig,
            uint8_t channel;
        );

        /** Returns configuration of the given channel. The actual configuration depends on the channel type
         */
        MESSAGE(ChannelConfig, 
            uint8_t channel;
            channel::Kind kind;
            uint8_t config[];
        );

        /** Channel configuration for the specified channel. The actual configuration type depends on the channel kind, which is also returned by the message.  
         */
        MESSAGE(SetChannelConfig,
            uint8_t channel;
            uint8_t config[];
        );

        /** Requests channel control information for given channel. 
         */
        MESSAGE(GetChannelControl,
            uint8_t channel;
        );

        /** Returns the channel control value. Returns the channel number, its kind and the control itself, whose value depends on the channel kind. 
         */
        MESSAGE(ChannelControl, 
            uint8_t channel;
            channel::Kind kind;
            uint8_t control[];
        );

        /** Explicitly sets control for the given channel(s). The data part consists of tuples (channel id, control) where control depends on the channel type. 
         */       
        MESSAGE(SetChannelControl,
            uint8_t data[];
        );

        MESSAGE(SetControlConsecutive, 
            uint8_t fromChannel;
            uint8_t numChannels;
            uint8_t data[];
        );

        /** Requests feedback value for the given channel. 
         */
        MESSAGE(GetChannelFeedback,
            uint8_t channel;
        );

        /** Returns the feedback value for the required channel together with its name and channel kind. The actual feedback depends on the channel kind. 
         */
        MESSAGE(ChannelFeedback, 
             uint8_t channel;
             channel::Kind kind;
             uint8_t feedback[];
        );

        /** Returns feedback information for selected channels. The data part consists of channel number followed by the feedback information for it, which can be repeated multiple times. Terminated by either 0 in the channel id position, or the end of the message (31 data bytes). 
         */
        MESSAGE(Feedback,
            uint8_t data[];
        );

        /** Returns feedback for N consecutive channels. 
         */
        MESSAGE(FeedbackConsecutive,
            uint8_t fromChannel;
            uint8_t numChannels;
            uint8_t data[];
        );

        /** Error Response. 
         
            TODO cause & stuff
         */
        MESSAGE(Error,
            uint8_t info;
        );

        /** Type of error returned. 
         */
        enum class ErrorKind : uint8_t {
            None = 0,
            DeviceNotPaired,
            InvalidCommand, 
            InvalidChannel,
            Unimplemented,
        };
    } // namespace remote::msg

#undef REQUEST
#undef RESPONSE

} // namespace remote