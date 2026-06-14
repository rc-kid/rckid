#include <rckid/rckid.h>
#include <rckid/hal.h>
#include <rckid/app.h>
#include <rckid/task.h>
#include <rckid/ui/header.h>
#include <rckid/audio/decoder_stream.h>
#include <rckid/apps/dialogs/info_dialog.h>

namespace rckid {

    DeviceState lastState_;
    DeviceState state_;

    uint16_t rgbRainbowHue_ = 0;

    uint32_t btnRepeat_[11] = { 
        RCKID_DEFAULT_KEY_REPEAT_MS,
        RCKID_DEFAULT_KEY_REPEAT_MS,
        RCKID_DEFAULT_KEY_REPEAT_MS,
        RCKID_DEFAULT_KEY_REPEAT_MS,    
        RCKID_DEFAULT_KEY_REPEAT_MS,
        RCKID_DEFAULT_KEY_REPEAT_MS,
        RCKID_DEFAULT_KEY_REPEAT_MS,
        RCKID_DEFAULT_KEY_REPEAT_MS,
        RCKID_DEFAULT_KEY_REPEAT_MS,
        RCKID_DEFAULT_KEY_REPEAT_MS,
        RCKID_DEFAULT_KEY_REPEAT_MS,
    };

    uint32_t btnRepeatCountdown_[11] = {
        RCKID_DEFAULT_KEY_REPEAT_MS / 16,
        RCKID_DEFAULT_KEY_REPEAT_MS / 16,
        RCKID_DEFAULT_KEY_REPEAT_MS / 16,
        RCKID_DEFAULT_KEY_REPEAT_MS / 16,
        RCKID_DEFAULT_KEY_REPEAT_MS / 16,
        RCKID_DEFAULT_KEY_REPEAT_MS / 16,
        RCKID_DEFAULT_KEY_REPEAT_MS / 16,
        RCKID_DEFAULT_KEY_REPEAT_MS / 16,
        RCKID_DEFAULT_KEY_REPEAT_MS / 16,
        RCKID_DEFAULT_KEY_REPEAT_MS / 16,
        RCKID_DEFAULT_KEY_REPEAT_MS / 16,
    };

    TinyDateTime now_;

    uint64_t nextSecondUptime_ = 0;

    struct DisplaySettings {
        uint8_t brightness = 128;
    } __attribute__((packed));

    struct AudioSettings {
        uint8_t volumeHeadphones = 8;
        uint8_t volumeSpeaker = 8;
    } __attribute__((packed));

    struct PimSettings {
        uint32_t budget = 610;
    } __attribute__((packed));

    struct RumblerSettings {
        uint8_t strength = 128;
        bool keyPress = true;
    } __attribute__((packed));

    struct RGBSettings {
        rgb::KeyboardEffect effect = rgb::KeyboardEffect::RainbowPress;
        Color color = Color::Green();
        uint8_t brightness = 32;
    } __attribute__((packed));
    
    struct Settings {
        static constexpr uint16_t VERSION = 1;
        uint16_t version = VERSION;
        DisplaySettings display;
        AudioSettings audio;
        PimSettings pim;
        RumblerSettings rumbler;
        RGBSettings rgb;
    } __attribute__((packed));

    Settings settings;

    // helpers

    void checkButtonRepeat(Btn btn) {
        // only matters if button is pressed
        if (! state_.button(btn))
            return;
        uint32_t const bi = static_cast<uint32_t>(btn);
        // repeat not enabled for the button
        if (btnRepeat_[bi] == 0)
            return;
        // reset the countdown if this is actual button press 
        if (! lastState_.button(btn)) {
            btnRepeatCountdown_[bi] = btnRepeat_[bi] / 16;
        } else {
            if (--btnRepeatCountdown_[bi] == 0) {
                lastState_.setButton(btn, false);
                // reset (the last state does not survive till next check)
                btnRepeatCountdown_[bi] = btnRepeat_[bi] / 16;
            }
        }
    }

    void buttonPressRGBEffect(Btn b) {
        switch (settings.rgb.effect) {
            case rgb::KeyboardEffect::RainbowPress:
            case rgb::KeyboardEffect::Press: {
                Color color;
                if (settings.rgb.effect == rgb::KeyboardEffect::RainbowPress) {
                    color = Color::HSV(rgbRainbowHue_ * 256, 255, settings.rgb.brightness);
                    rgbRainbowHue_ += 13;                
                } else {
                    color = settings.rgb.color.withBrightness(settings.rgb.brightness);
                }
                rgb::setBtnEffect(b, RGBEffect::Solid(color, 255, 0));
                rgb::setBtnEffect(b, RGBEffect::Solid(Color::Black()));
                break;
            }
            default:
                break;
        }
    }

    // device

    void initialize() {
        hal::device::initialize();
        hal::storage::load(0, reinterpret_cast<uint8_t *>(& settings), sizeof(settings));
        if (settings.version != Settings::VERSION) {
            LOG(LL_WARN, "Settings version mismatch, resetting to defaults");
            settings = Settings{};
        }
        now_ = hal::time::now();
        nextSecondUptime_ = hal::time::uptimeUs() + 1000000;
        // TODO


        // set the keyboard effect (if any)
        rgb::setKeyboardEffect(settings.rgb.effect, settings.rgb.color);
    }

    void tick() {
        // move to next state
        lastState_ = state_;
        state_.updateWith(hal::io::state());
        bool buttonPressed = false;
        // update button repeat intervals
        for (uint32_t i = 0; i < 11; ++i) {
            Btn b = static_cast<Btn>(i);
            if (btnPressed(b)) {
                buttonPressed = true;
                buttonPressRGBEffect(b);
            }
            checkButtonRepeat(static_cast<Btn>(i));
        }
        // if we have new button press (any button), nudge
        if (buttonPressed && settings.rumbler.keyPress)
            rumbler::nudge();
        // check state interrupts
        if (state_.powerOffInterrupt()) {
            LOG(LL_INFO, "Power off requested");
            // TODO acknowledge power off request
            power::powerOff();
        }
        // TODO wakeup interrupt
        // TODO accel interrupt
        state_.clearInterrupts();
        // run hal's on tick & yield (this likely requests new state to be gathered as well)
        hal::device::onTick();
        hal::device::onYield();
        // check if we need to trigger second tick
        if (hal::time::uptimeUs() >= nextSecondUptime_) {
            nextSecondUptime_ += 1000000;
            now_.inc();
            ui::Header::update();
            if (App::current() != nullptr) {
                if (App::current()->capabilities().consumesBudget && pim::updateBudget(-1) == 0) {
                    InfoDialog::error("Out of budget", "No more budget today to play the game.");
                    App::current()->exit();
                }
            } 
        }
        // run tasks
        Task::runAll();
    }

    // io

    bool btnDown(Btn btn) {
        return state_.button(btn);
    }

    bool btnPressed(Btn btn) {
        return state_.button(btn) && !lastState_.button(btn);
    }

    bool btnReleased(Btn btn) {
        return !state_.button(btn) && lastState_.button(btn);
    }

    void btnClear(Btn btn) {
        // simply ensure last state is identical to current state
        lastState_.setButton(btn, btnDown(btn));
    }

    void btnClearAll() {
        lastState_ = state_;
    }

    uint32_t btnRepeat(Btn btn) {
        return btnRepeat_[static_cast<uint32_t>(btn)];
    }

    void btnSetRepeat(Btn btn, uint32_t repeat_ms) {
        btnRepeat_[static_cast<uint32_t>(btn)] = repeat_ms;
        btnRepeatCountdown_[static_cast<uint32_t>(btn)] = repeat_ms / 16;
    }

    namespace power {

        void powerOff() {
            // exit all applications (saving their state)
            App::onPowerOff();
            // save settings to the storage on AVR for persistence
            hal::storage::save(0, reinterpret_cast<uint8_t const *>(& settings), sizeof(Settings));
            // and finally, power off
            hal::device::powerOff();
        }

        uint32_t vcc() {
            return state_.vcc();
        }

        uint32_t batteryLevel() {
            uint32_t v = state_.vcc();
            if (v >= 420)
                return 100;
            else if (v >= 300)
                return (v - 300) * 100 / (120);
            else
                return 0;
        }

        bool charging() {
            return state_.charging();
        }

        bool dcConnected() {
            return (state_.vcc() >= 450);
        }
    }

    // time

    namespace time {

        TinyDateTime now() {
            return now_;
        }
    } // namespace rckid::time

    // debug

    namespace debug {

        bool readAndAddHexDigit(uint32_t & value) {
            uint8_t x = read();
            if (x >= '0' && x <= '9')
                x = x - '0';
            else if (x >= 'a' && x <= 'f')
                x = x - 'a' + 10;
            else if (x >= 'A' && x <= 'F')
                x = x - 'A' + 10;
            else 
                return false;
            value = (value << 4) | x;
            return true;
        }

        uint8_t readHex8() {
            uint32_t result = 0;
            if (!readAndAddHexDigit(result))
                return result;
            readAndAddHexDigit(result);
            return result;
        }

        uint16_t readHex16() {
            uint32_t result = 0;
            if (!readAndAddHexDigit(result))
                return result;
            if (!readAndAddHexDigit(result))
                return result;
            if (!readAndAddHexDigit(result))
                return result;
            readAndAddHexDigit(result);
            return result;
        }

    } // namespace rckid::debug

    // display

    namespace display {
        Rect rect_;
        RefreshDirection refreshDirection_;

        void enable(Rect rect, RefreshDirection  direction) {
            ASSERT(Rect::WH(WIDTH, HEIGHT).contains(rect));
            waitUpdateDone();
            hal::display::enable(rect, direction);
            rect_ = rect;
            refreshDirection_ = direction;
        }

        uint8_t brightness() { return settings.display.brightness; }

        void setBrightness(uint8_t value) {
            hal::display::setBrightness(value);
            settings.display.brightness = value;
        }

    } // namespace rckid::display

    // audio

    namespace audio {

        void play(DecoderStream * stream) {
            // ensure the stream's playback buffer is filled
            stream->update();
            // start the playback
            play(stream->sampleRate(), [stream](int16_t * & buffer, uint32_t & stereoSamples) {
                stream->callback(buffer, stereoSamples);
            });
        }

        bool headphonesConnected() {
            return state_.headphonesConnected();
        }

        uint8_t volume() {
            return headphonesConnected() ? settings.audio.volumeHeadphones : settings.audio.volumeSpeaker;
        }

        void setVolume(uint8_t value) {
            if (value == 255) // 0 - 1
                value = 0;
            if (value > 15)
                value = 15;
            if (headphonesConnected()) {
                settings.audio.volumeHeadphones = value;
                hal::audio::setVolumeHeadphones(value);
            } else {
                settings.audio.volumeSpeaker = value;
                hal::audio::setVolumeSpeaker(value);
            }
            ui::Header::update();
        }

    } // namespace rckid::audio

    namespace pim {

        uint32_t remainingBudget() { return settings.pim.budget; }

        uint32_t updateBudget(int32_t value) {
            if (value > 0) {
                settings.pim.budget += value;
            } else {
                uint32_t absValue = static_cast<uint32_t>(-value);
                if (absValue > settings.pim.budget)
                    settings.pim.budget = 0;
                else
                    settings.pim.budget -= absValue;
            }
            return settings.pim.budget;
        }

    } // namespace rckid::pim

    namespace rumbler {

        uint8_t strength() { return settings.rumbler.strength >> 4; }

        void setStrength(uint8_t value) { settings.rumbler.strength = (value << 4) | value; }

        void nudge() { 
            hal::rumbler::setEffect(RumblerEffect{
                /* strength */ settings.rumbler.strength, 
                /* timeOn */   5,
                /* timeOff */  0,
                /* cycles */   1
            }); 
        }

        void success() { 
            hal::rumbler::setEffect(RumblerEffect{
                /* strength */ 192, 
                /* timeOn */   5,
                /* timeOff */  0,
                /* cycles */   1
            }); 
        }

        void fail() { 
            hal::rumbler::setEffect(RumblerEffect{
                /* strength */ 255, 
                /* timeOn */   5,
                /* timeOff */  5,
                /* cycles */   3
            }); 
        }

    } // namespace rckid::rumbler

    namespace rgb {

        void off() {
            hal::rgb::setEffectAll(RGBEffect::Off());
        }

        uint8_t brightness() { return settings.rgb.brightness; }

        void setBrightness(uint8_t value) { 
            settings.rgb.brightness = value;
            setKeyboardEffect(settings.rgb.effect, settings.rgb.color);
        }

        Color color() { return settings.rgb.color; }

        void setColor(Color color) {
            settings.rgb.color = color;
            setKeyboardEffect(settings.rgb.effect, settings.rgb.color);
        }

        KeyboardEffect keyboardEffect() { return settings.rgb.effect; }

        void setKeyboardEffect(KeyboardEffect effect, Color color) {
            settings.rgb.effect = effect;
            settings.rgb.color = color;
            // issue the RGB commands based on the effect & settings
            switch (effect) {
                case KeyboardEffect::Off:
                // turn all LEDs off for press effects as well (we'll be turning them during state changes)
                case KeyboardEffect::Press:
                case KeyboardEffect::RainbowPress:
                    hal::rgb::setEffectAll(RGBEffect::Off());
                    break;
                case KeyboardEffect::Solid:
                    hal::rgb::setEffectAll(RGBEffect::Solid(color.withBrightness(settings.rgb.brightness)));
                    break;
                case KeyboardEffect::Breathe:
                    hal::rgb::setEffectAll(RGBEffect::Breathe(color.withBrightness(settings.rgb.brightness)));
                    break;
                case KeyboardEffect::Rainbow:
                    hal::rgb::setEffectAll(RGBEffect::Rainbow(0, 1, 4, settings.rgb.brightness));
                    break;
                case KeyboardEffect::RainbowWave:
                    hal::rgb::setEffect(0, RGBEffect::Rainbow(0, 1, 4, settings.rgb.brightness));
                    hal::rgb::setEffect(1, RGBEffect::Rainbow(0, 1, 4, settings.rgb.brightness));
                    hal::rgb::setEffect(2, RGBEffect::Rainbow(0, 1, 4, settings.rgb.brightness));
                    hal::rgb::setEffect(3, RGBEffect::Rainbow(0, 1, 4, settings.rgb.brightness));
                    hal::rgb::setEffect(4, RGBEffect::Rainbow(51, 1, 4, settings.rgb.brightness));
                    hal::rgb::setEffect(5, RGBEffect::Rainbow(102, 1, 4, settings.rgb.brightness));
                    hal::rgb::setEffect(6, RGBEffect::Rainbow(153, 1, 4, settings.rgb.brightness));
                    hal::rgb::setEffect(7, RGBEffect::Rainbow(204, 1, 4, settings.rgb.brightness));
                    break;
                default:
                    UNREACHABLE;
            }
        }

        void setBtnEffect(Btn btn, RGBEffect const & effect) {
            switch (btn) {
                case Btn::Up:
                case Btn::Down:
                case Btn::Left:
                case Btn::Right:
                    hal::rgb::setEffect(0, effect);
                    hal::rgb::setEffect(1, effect);
                    hal::rgb::setEffect(2, effect);
                    hal::rgb::setEffect(3, effect);
                    break;
                case Btn::Select:
                    hal::rgb::setEffect(4, effect);
                    break;
                case Btn::Start:
                    hal::rgb::setEffect(5, effect);
                    break;
                case Btn::B:
                    hal::rgb::setEffect(6, effect);
                    break;
                case Btn::A:
                    hal::rgb::setEffect(7, effect);
                    break;
                // side buttons (home & volume keys) do not have lights
                default:
                    break;
            }
        }

    } // namespace rckid::rgb

    // debugging

    // hal layer events

    void onFatalError(char const * file, uint32_t line, char const * msg, uint32_t payload) {
        LOG(LL_ERROR, "Fatal error at " << file << ":" << line << "\n" << msg << " (payload " << payload << ")");
        // TODO do the BSOD

        // infinite loop so that we never return
        while (true)
            yield();
    }
}
