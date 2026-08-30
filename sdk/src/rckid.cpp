#include <rckid/rckid.h>
#include <rckid/hal.h>
#include <rckid/app.h>
#include <rckid/task.h>
#include <rckid/ui/header.h>
#include <rckid/audio/decoder_stream.h>
#include <rckid/apps/dialogs/info_dialog.h>
#include <rckid/apps/unlock.h>
#include <rckid/graphics/tile_grid.h>
#include <rckid/serialization.h>

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

    // FPS counter, which is reset every second
    uint32_t fps_ = 0;

    struct DisplaySettings {
        uint8_t brightness = 128;
    };

    struct AudioSettings {
        uint8_t volumeHeadphones = 8;
        uint8_t volumeSpeaker = 8;
    };

    struct PimSettings {
        uint32_t budget = 610;
        String password;
        String parentPassword;
    };

    struct RumblerSettings {
        uint8_t strength = 128;
        bool keyPress = true;
    };

    struct RGBSettings {
        uint8_t brightness = 32;
    };
    
    /** Device settings. 

        Similar to the ui::Style, settings define the device configuration. Settings are stored in the device's non-volatime memory and are expected to survive device reboots. They are mainly used for non-ui/visual configuration of more ad-hoc, or device specific nature such as display brightness, audio volume, etc.
     */
    struct Settings {
        static constexpr uint16_t VERSION = 3;
        uint16_t version = VERSION;
        DisplaySettings display;
        AudioSettings audio;
        PimSettings pim;
        RumblerSettings rumbler;
        RGBSettings rgb;
    };

    void write(BinaryWriter & w, Settings const & settings) {
        w << settings.version
          << settings.display.brightness
          << settings.audio.volumeHeadphones
          << settings.audio.volumeSpeaker
          << settings.pim.budget
          << settings.pim.password
          << settings.pim.parentPassword
          << settings.rumbler.strength
          << settings.rumbler.keyPress
          << settings.rgb.brightness;
    }

    void read(BinaryReader & r, Settings & settings) {
        r >> settings.version
          >> settings.display.brightness
          >> settings.audio.volumeHeadphones
          >> settings.audio.volumeSpeaker
          >> settings.pim.budget
          >> settings.pim.password
          >> settings.pim.parentPassword
          >> settings.rumbler.strength
          >> settings.rumbler.keyPress
          >> settings.rgb.brightness;
    }

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
        switch (ui::Style::keyboardEffect()) {
            case rgb::KeyboardEffect::RainbowPress:
            case rgb::KeyboardEffect::Press: {
                Color color;
                if (ui::Style::keyboardEffect() == rgb::KeyboardEffect::RainbowPress) {
                    color = Color::HSV(rgbRainbowHue_ * 256, 255, settings.rgb.brightness);
                    rgbRainbowHue_ += 13;                
                } else {
                    color = ui::Style::keyboardRGBColor().withBrightness(settings.rgb.brightness);
                }
                rgb::setBtnEffect(b, RGBEffect::Solid(color, 255, 0));
                rgb::setBtnEffect(b, RGBEffect::Solid(Color::Black()));
                break;
            }
            default:
                break;
        }
    }

    void loadSettings() {
        MemoryStream s = MemoryStream::withCapacity(1024);
        hal::storage::load(0, s.data(), s.size());
        s.binaryReader() >> settings;
        if (settings.version != Settings::VERSION) {
            LOG(LL_WARN, "Settings version mismatch, resetting to defaults");
            settings = Settings{};
        }
    }

    void saveSettings() {
        MemoryStream s = MemoryStream::withCapacity(1024);
        s.binaryWriter() << settings;
        LOG(LL_INFO, "Saving settings, size " << s.tell());
        hal::storage::save(0, s.data(), s.tell());
    }

    // device

    void initialize() {
        hal::device::initialize();
        loadSettings();
        now_ = hal::time::now();
        nextSecondUptime_ = hal::time::uptimeUs() + 1000000;
        // TODO
        LOG(LL_INFO, "Brightness " << settings.display.brightness);
        hal::display::setBrightness(settings.display.brightness);

        // set default volume headphones
        hal::audio::setVolumeHeadphones(settings.audio.volumeHeadphones);
        hal::audio::setVolumeSpeaker(settings.audio.volumeHeadphones);

        // ensure default style is initialized (and loaded from SD card if available)
        ui::Style::loadDefaultStyle();

        // set the keyboard effect (if any)
        rgb::setKeyboardEffect(ui::Style::keyboardEffect(), ui::Style::keyboardRGBColor());

        // see if we need password protection
        if (! settings.pim.password.empty())
            App::run<Unlock>(settings.pim.password);
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
            LOG(LL_FPS, "FPS " << fps_);
            fps_ = 0;
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
            saveSettings();
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

        void setTime(TinyDateTime dt) {
            now_ = dt;
            hal::time::setTime(dt);
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

        bool debugMode() {
            return state_.debugMode();
        }

        void setDebugMode(bool value) {
            state_.setDebugMode(value);
            hal::device::setDebugMode(value);
        }

    } // namespace rckid::debug

    // display

    namespace display {
        Rect rect_;
        RefreshDirection refreshDirection_;

        void waitVSync() {
            while (hal::display::vSync())
                yield();
            while (! hal::display::vSync())
                yield();
            ++fps_;
        }

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
            // limit volume to 0..15
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

        void setPassword(String password) {
            settings.pim.password = std::move(password);
            // just to be sure - normally this is done at exit
            saveSettings();
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

        uint8_t brightness() { return settings.rgb.brightness >> 4; }

        void setBrightness(uint8_t value) { 
            settings.rgb.brightness = (value << 4) | value;
        }

        void setKeyboardEffect(KeyboardEffect effect, Color color) {
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

        void write(Writer & w, KeyboardEffect const & effect) {
            switch (effect) {
                case KeyboardEffect::Off: 
                    w << "Off"; 
                    break;
                case KeyboardEffect::Press: 
                    w << "Press"; 
                    break;
                case KeyboardEffect::RainbowPress: 
                    w << "RainbowPress"; 
                    break;
                case KeyboardEffect::Solid: 
                    w << "Solid"; 
                    break;
                case KeyboardEffect::Breathe: 
                    w << "Breathe"; 
                    break;
                case KeyboardEffect::Rainbow: 
                    w << "Rainbow"; 
                    break;
                case KeyboardEffect::RainbowWave: 
                    w << "RainbowWave"; 
                    break;
                default:
                    UNREACHABLE;
                    break;
            }
        }

        void read(Reader & r, KeyboardEffect & effect) {
            String s;
            r >> s;
            if (s == "Off") 
                effect = KeyboardEffect::Off;
            else if (s == "Press") 
                effect = KeyboardEffect::Press;
            else if (s == "RainbowPress") 
                effect = KeyboardEffect::RainbowPress;
            else if (s == "Solid") 
                effect = KeyboardEffect::Solid;
            else if (s == "Breathe") 
                effect = KeyboardEffect::Breathe;
            else if (s == "Rainbow") 
                effect = KeyboardEffect::Rainbow;
            else if (s == "RainbowWave") 
                effect = KeyboardEffect::RainbowWave;
            else
                UNREACHABLE;
        }   


    } // namespace rckid::rgb

    // debugging

    // hal layer events

    void onFatalError(char const * file, uint32_t line, char const * msg, uint32_t payload) {
        LOG(LL_ERROR, "Fatal error at " << file << ":" << line << "\n" << msg << " (payload " << payload << ")");
        // TODO do the BSOD
        Color::RGB565 palette[] = {
            Color::RGB(0x00, 0x00, 0xff),
            Color::RGB(0x11, 0x11, 0xff),
            Color::RGB(0x22, 0x22, 0xff),
            Color::RGB(0x33, 0x33, 0xff),
            Color::RGB(0x44, 0x44, 0xff),
            Color::RGB(0x55, 0x55, 0xff),
            Color::RGB(0x66, 0x66, 0xff),
            Color::RGB(0x77, 0x77, 0xff),
            Color::RGB(0x88, 0x88, 0xff),
            Color::RGB(0x99, 0x99, 0xff),
            Color::RGB(0xaa, 0xaa, 0xff),
            Color::RGB(0xbb, 0xbb, 0xff),
            Color::RGB(0xcc, 0xcc, 0xff),
            Color::RGB(0xdd, 0xdd, 0xff),
            Color::RGB(0xee, 0xee, 0xff),
            Color::RGB(0xff, 0xff, 0xff),
        };
        TileGrid g{40,15, palette};
        g.text(0,1) 
            << ":( RCKid fatal error\n"
            << "   " << file << ":" << line << "\n"
            << "   " << msg << "\n"
            << "   (payload " << payload << ")";
        
        // send the BSOD tile grid data to the display
        Color::RGB565 buffer[240];
        for (Coord i = 319; i >= 0; --i) {
            yield();
            memset16(reinterpret_cast<uint16_t *>(buffer), palette[0], 240);
            yield();
            g.renderColumn(i, 0, buffer, 240);
            yield();
            hal::display::update(buffer, 240);
            while (hal::display::updateActive())
                yield();
        }
        // infinite loop so that we never return
        while (true)
            yield();
    }

} // namespace rckid
