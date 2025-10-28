#pragma once

#include "../graphics/color.h"
#include "../graphics/icon.h"
#include "../assets/images.h"

namespace rckid::ui {

    /** Basic UI style for the device. 
     
        The style consists of a background image, default text and background colors and two accent colors (foreground and background).
     */
    class Style {
    public:

        static constexpr char const * STYLE_SETTINGS_FILE = "style.ini";
        static constexpr char const * SECTION_DEFAULT = "default";
        static constexpr char const * SECTION_ACCENT = "accent";
        static constexpr char const * SECTION_BACKGROUND = "background";

        static ColorRGB fg() { return fg_; }
        static ColorRGB bg() { return bg_; }
        static ColorRGB accentFg() { return accentFg_; }
        static ColorRGB accentBg() { return accentBg_; }
        static Icon const & background() { return background_; }
        
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

        static void load();

        static Bitmap loadBackgroundImage() {
            char const * path = background_.filename();
            if (path != nullptr && ! fs::isFile(path))
                return Icon{assets::background16}.toBitmap();
            return background_.toBitmap();
        }

        static void save();

        static void refresh();

        static void saveAndRefresh() { save(); refresh(); }

    private:


        static inline Icon background_{assets::background16};
        static inline ColorRGB fg_{ColorRGB::White()};
        static inline ColorRGB bg_{ColorRGB::RGB(32, 32, 32)};
        static inline ColorRGB accentFg_{ColorRGB::RGB(0, 255, 0)};
        static inline ColorRGB accentBg_{ColorRGB::RGB(16, 16, 16)};

    }; // rckid::ui::Style

} // rckid::ui