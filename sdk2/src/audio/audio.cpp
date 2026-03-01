#include <rckid/ui/header.h>
#include <rckid/audio/audio.h>

namespace rckid {
    extern hal::State state_;
} // namespace rckid

namespace rckid::audio {

    namespace {
        uint8_t headphonesVolume_ = 8;
        uint8_t speakerVolume_ = 8;
    }
    
    bool headphonesConnected() {
        return state_.headphonesConnected();
    }

    uint8_t volume() {
        return headphonesConnected() ? headphonesVolume_ : speakerVolume_;
    }

    void setVolume(uint8_t value) {
        if (value == 255) // 0 - 1
            value = 0;
        if (value > 15)
            value = 15;
        if (headphonesConnected()) {
            headphonesVolume_ = value;
            hal::audio::setVolumeHeadphones(value);
        } else {
            speakerVolume_ = value;
            hal::audio::setVolumeSpeaker(value);
        }
        ui::Header::update();
    }
} // namespace rckid::audio
