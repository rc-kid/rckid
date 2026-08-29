#pragma once

#include <rckid/graphics/color.h>
#include <rckid/graphics/image_source.h>
#include <rckid/ini.h>

#include <assets/images.h>

namespace rckid::ui {

    /** Style for ui widgets. 
     
        Styles are object that define general widget visualization properties, such as colors, accents, fonts, etc. Widgets then support applying styles to themselves. The styling is simple, as the style merely holds the properties and the widgets determines what properties from the style to apply and how. 

        A single global style is suppported at any time, but applications and widgets do not implicitlyrestyle on a style change - styling is always explicit, either in the constructor, or via the applyStyle method of ui::Widget.

        System (default) style is automatically managed by the SDK (loaded during initialization and saved on change) from the SD card.
     */
    class Style {
    public:

        static void loadDefaultStyle();
        static void saveDefaultStyle();

        static void load(ini::Reader & reader);

        static void save(ini::Writer & writer);

        static Color defaultFg() { return defaultFg_; }
        static Color defaultBg() { return defaultBg_; }

        static void setDefaultFg(Color value) { defaultFg_ = value; }
        static void setDefaultBg(Color value) { defaultBg_ = value; }

        static Color accentFg() { return accentFg_; }
        static Color accentBg() { return accentBg_; }

        static void setAccentFg(Color value) { accentFg_ = value; }
        static void setAccentBg(Color value) { accentBg_ = value; }

        static uint32_t animationSpeed() { return animationSpeed_; }

        static ImageSource const & backgroundImage() { return backgroundImage_; }

        static void setBackgroundImage(ImageSource img) {
            backgroundImage_ = std::move(img);
        }

        static rgb::KeyboardEffect keyboardEffect() { return keyboardEffect_; }
        static void setKeyboardEffect(rgb::KeyboardEffect effect) { keyboardEffect_ = effect; }

        static Color keyboardRGBColor() { return keyboardRGBColor_; }
        static void setKeyboardRGBColor(Color color) { keyboardRGBColor_ = color; }

    private:
        static constexpr char const * DEFAULT_FILE = "style2.ini";

        static inline Color defaultFg_ = Color::White();
        static inline Color defaultBg_ = Color::Black();
        
        static inline Color accentFg_ = Color::White();
        static inline Color accentBg_ = Color::RGB(32, 32, 32);
        
        static inline uint32_t animationSpeed_ = RCKID_DEFAULT_ANIMATION_DURATION_MS;

        static inline ImageSource backgroundImage_{assets::images::logo};

        static inline rgb::KeyboardEffect keyboardEffect_ = rgb::KeyboardEffect::RainbowPress;
        static inline Color keyboardRGBColor_ = Color::RGB(255, 255, 255);

    }; // rckid::ui::Style

} // namespace rckid::ui