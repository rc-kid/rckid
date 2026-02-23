#include <rckid/ui/header.h>
#include <rckid/audio/audio.h>

namespace rckid::audio {

    namespace {
        bool headphonesConnected_ = false;
        uint8_t headphonesVolume_ = 8;
        uint8_t speakerVolume_ = 8;
    }
    
    bool headphonesConnected() {
        return headphonesConnected_;
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
}

namespace rckid {
    void onHeadphonesChange(bool connected) {
        if (audio::headphonesConnected_ == connected)
            return;
        audio::headphonesConnected_ = connected;
        ui::Header::update();
    }
}