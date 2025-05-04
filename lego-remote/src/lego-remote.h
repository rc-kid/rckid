#include "remote.h"

namespace remote {

    class LegoRemote {
    public:
        static constexpr uint8_t NUM_CHANNELS = 16;

        static constexpr uint8_t CHANNEL_DEVICE = 1;
        static constexpr uint8_t CHANNEL_ML = 2;
        static constexpr uint8_t CHANNEL_MR = 3;
        static constexpr uint8_t CHANNEL_L1 = 4;
        static constexpr uint8_t CHANNEL_R1 = 5;
        static constexpr uint8_t CHANNEL_L2 = 6;
        static constexpr uint8_t CHANNEL_R2 = 7;
        static constexpr uint8_t CHANNEL_TONE_EFFECT = 8;
        static constexpr uint8_t CHANNEL_RGB_STRIP = 9;

        /** The channel info response as it is always the same. 
         */
        static constexpr uint8_t CHANNEL_INFO[] = {
            msg::ChannelInfo::ID,
            static_cast<uint8_t>(channel::Kind::Device),
            static_cast<uint8_t>(channel::Kind::Motor),
            static_cast<uint8_t>(channel::Kind::Motor),
            static_cast<uint8_t>(channel::Kind::CustomIO),
            static_cast<uint8_t>(channel::Kind::CustomIO),
            static_cast<uint8_t>(channel::Kind::CustomIO),
            static_cast<uint8_t>(channel::Kind::CustomIO),
            static_cast<uint8_t>(channel::Kind::ToneEffect),
            static_cast<uint8_t>(channel::Kind::RGBStrip),
            static_cast<uint8_t>(channel::Kind::RGBColor),
            static_cast<uint8_t>(channel::Kind::RGBColor),
            static_cast<uint8_t>(channel::Kind::RGBColor),
            static_cast<uint8_t>(channel::Kind::RGBColor),
            static_cast<uint8_t>(channel::Kind::RGBColor),
            static_cast<uint8_t>(channel::Kind::RGBColor),
            static_cast<uint8_t>(channel::Kind::RGBColor),
            static_cast<uint8_t>(channel::Kind::RGBColor),
            0 // end of channels to send 
        }; 

        /** Since the entire feedback of the Lego remote fits in 32 bytes, we can simply send all */
        class Feedback {
        private:
            uint8_t const msg = msg::FeedbackConsecutive::ID;
            uint8_t const startChannel = 1;
            uint8_t const numChannels = 9;
        public:
            channel::Device::Feedback device;
            channel::Motor::Feedback ml;
            channel::Motor::Feedback mr;
            channel::CustomIO::Feedback l1;
            channel::CustomIO::Feedback r1; 
            channel::CustomIO::Feedback l2;
            channel::CustomIO::Feedback r2;
            channel::ToneEffect::Feedback tone;
            channel::RGBStrip::Feedback rgb;
        } __attribute((__packed__)); // LegoRemote::Feedback
        
        static_assert(sizeof(Feedback) <= 32);

    }; // LegoRemote

}; // namespace remote