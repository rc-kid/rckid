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
        static ColorRGB fg() { return fg_; }
        static ColorRGB bg() { return bg_; }
        static ColorRGB accentFg() { return accentFg_; }
        static ColorRGB accentBg() { return accentBg_; }
        static Icon const & background() { return background_; }
        
        static void setFg(ColorRGB color) {
            if (color == fg_)
                return;
            fg_ = color;
            save();
        }

        static void setBg(ColorRGB color) {
            if (color == bg_)
                return;
            bg_ = color;
            save();
        }

        static void setAccentFg(ColorRGB color) {
            if (color == accentFg_)
                return;
            accentFg_ = color;
            save();
        }

        static void setAccentBg(ColorRGB color) {
            if (color == accentBg_)
                return;
            accentBg_ = color;
            save();
        }

        static void setBackground(Icon icon) {
            background_ = Icon{std::move(icon)};
            save();
        }

        static void load() {
            // TODO load from persistent storage
        }

        static Bitmap loadBackgroundImage() {
            char const * path = background_.filename();
            if (path != nullptr && ! fs::isFile(path))
                return Icon{assets::background16}.toBitmap();
            return background_.toBitmap();
        }

    private:

        static void save() {
            // TODO save to persistent storage
        }

        static inline Icon background_{assets::background16};
        static inline ColorRGB fg_{ColorRGB::White()};
        static inline ColorRGB bg_{ColorRGB::RGB(32, 32, 32)};
        static inline ColorRGB accentFg_{ColorRGB::RGB(0, 128, 0)};
        static inline ColorRGB accentBg_{ColorRGB::RGB(16, 16, 16)};

    }; // rckid::ui::Style

} // rckid::ui