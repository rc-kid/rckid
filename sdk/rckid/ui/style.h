#pragma once

#include "../graphics/color.h"
#include "../graphics/icon.h"
#include "../assets/images.h"

namespace rckid::ui {

    /** RGB button visuals. 
     */
    enum class RGBStyle : uint8_t {
        Off,
        Rainbow, 
        RainbowWave,
        Breathe,
        BreatheWave,
        Solid,
        Key,
        RainbowKey,
    }; 

    enum class BackgroundScrollStyle : uint8_t {
        Off,
        Scroll,
        Nudge,
    };

    /** Basic UI style for the device. 
     
        The style consists of a background image, default text and background colors and two accent colors (foreground and background).
     */
    class Style {
    public:


        static constexpr char const * STYLE_SETTINGS_FILE = "style.ini";
        static constexpr char const * SECTION_DEFAULT = "default";
        static constexpr char const * SECTION_ACCENT = "accent";
        static constexpr char const * SECTION_BACKGROUND = "background";
        static constexpr char const * SECTION_RGB = "rgb";
        static constexpr char const * SECTION_RUMBLER = "rumbler";

        static ColorRGB fg() { return fg_; }
        static ColorRGB bg() { return bg_; }
        static ColorRGB accentFg() { return accentFg_; }
        static ColorRGB accentBg() { return accentBg_; }
        static Icon const & background() { return background_; }
        static uint8_t rgbBrightness() { return rgbBrightness_; }
        static RGBStyle rgbStyle() { return rgbStyle_; }
        static ColorRGB rgbColor() { return rgbColor_; }
        static BackgroundScrollStyle backgroundScroll() { return backgroundScroll_; }
        static bool backgroundTilt() { return backgroundTilt_; }

        static uint8_t rumblerKeyPressIntensity() { return rumblerKeyPressIntensity_; }
        
        static void setFg(ColorRGB color) {
            if (color == fg_)
                return;
            fg_ = color;
        }

        static void setBg(ColorRGB color) {
            if (color == bg_)
                return;
            bg_ = color;
        }

        static void setAccentFg(ColorRGB color) {
            if (color == accentFg_)
                return;
            accentFg_ = color;
        }

        static void setAccentBg(ColorRGB color) {
            if (color == accentBg_)
                return;
            accentBg_ = color;
        }

        static void setBackground(Icon icon) {
            background_ = Icon{std::move(icon)};
        }

        static void setBackgroundScroll(BackgroundScrollStyle style) {
            backgroundScroll_ = style;
        }

        static void setBackgroundTilt(bool value) {
            backgroundTilt_ = value;
        }

        static void setRgbBrightness(uint8_t value) {
            rgbBrightness_ = value;
            showRGBStyle();
        }

        static void setRgbStyle(RGBStyle style) {
            rgbStyle_ = style;
            showRGBStyle();
        }

        static void setRgbColor(ColorRGB color) {
            rgbColor_ = color;
            showRGBStyle();
        }

        static void setRumblerKeyPressIntensity(uint8_t intensity) {
            rumblerKeyPressIntensity_ = intensity;
        }

        static void load();

        static Bitmap loadBackgroundImage() {
            char const * path = background_.filename();
            if (path != nullptr && ! fs::isFile(path))
                return Icon{assets::background16}.toBitmap();
            return background_.toBitmap();
        }

        static void showRGBStyle();

        static void save();

        static void refresh();

        /** Refresh the style first, only then save so that any errors during the refresh will not propagate to the saving and brick the device.
         */
        static void refreshAndSave() { refresh(); save(); }

    private:

        static inline Icon background_{assets::background16};
        static inline BackgroundScrollStyle backgroundScroll_ = BackgroundScrollStyle::Nudge;
        static inline bool backgroundTilt_ = true;
        static inline ColorRGB fg_{ColorRGB::White()};
        static inline ColorRGB bg_{ColorRGB::RGB(32, 32, 32)};
        static inline ColorRGB accentFg_{ColorRGB::RGB(0, 255, 0)};
        static inline ColorRGB accentBg_{ColorRGB::RGB(16, 16, 16)};
        static inline uint8_t rgbBrightness_ = RCKID_RGB_BRIGHTNESS;
        static inline RGBStyle rgbStyle_ = RGBStyle::Rainbow;
        static inline ColorRGB rgbColor_ = ColorRGB::White();

        static inline uint8_t rumblerKeyPressIntensity_ = RCKID_RUMBLER_NUDGE_STRENGTH;

    }; // rckid::ui::Style

    inline Writer & operator << (Writer & w, RGBStyle style) {
        switch (style) {
            case RGBStyle::Off:
                w << "off";
                break;
            case RGBStyle::Rainbow:
                w << "rainbow";
                break;
            case RGBStyle::RainbowWave:
                w << "rainbowWave";
                break;
            case RGBStyle::Breathe:
                w << "breathe";
                break;
            case RGBStyle::BreatheWave:
                w << "breatheWave";
                break;
            case RGBStyle::Solid:
                w << "solid";
                break;
            case RGBStyle::Key:
                w << "key";
                break;
            case RGBStyle::RainbowKey:
                w << "rainbowKey";
                break;
            default:
                UNREACHABLE;
        }
        return w;
    }

    inline RGBStyle rgbStyleFromString(String const & str) {
        if (str == "rainbow")
            return RGBStyle::Rainbow;
        else if (str == "rainbowWave")
            return RGBStyle::RainbowWave;
        else if (str == "breathe")
            return RGBStyle::Breathe;
        else if (str == "breatheWave")
            return RGBStyle::BreatheWave;
        else if (str == "solid")
            return RGBStyle::Solid;
        else if (str == "key")
            return RGBStyle::Key;
        else if (str == "rainbowKey")
            return RGBStyle::RainbowKey;
        else if (str == "off")
            return RGBStyle::Off;
        else {
            LOG(LL_ERROR, "Invalid RGB style " << str);
            return RGBStyle::Off;
        }
    }

    inline Writer & operator << (Writer & w, BackgroundScrollStyle style) {
        switch (style) {
            case BackgroundScrollStyle::Off:
                w << "off";
                break;
            case BackgroundScrollStyle::Scroll:
                w << "scroll";
                break;
            case BackgroundScrollStyle::Nudge:
                w << "nudge";
                break;
            default:
                UNREACHABLE;
        }
        return w;
    }

    inline BackgroundScrollStyle backgroundScrollStyleFromString(String const & str) {
        if (str == "scroll")
            return BackgroundScrollStyle::Scroll;
        else if (str == "nudge")
            return BackgroundScrollStyle::Nudge;
        else if (str == "off")
            return BackgroundScrollStyle::Off;
        else {
            LOG(LL_ERROR, "Invalid background scroll style " << str);
            return BackgroundScrollStyle::Off;
        }
    }

} // rckid::ui