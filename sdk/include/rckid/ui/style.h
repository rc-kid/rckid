#pragma once

#include <rckid/graphics/color.h>
#include <rckid/graphics/image_source.h>
#include <rckid/ini.h>

#include <assets/images.h>

namespace rckid::ui {

    /** Style for ui widgets. 
     
        Styles are object that define general widget visualization properties, such as colors, accents, fonts, etc. Widgets then support applying styles to themselves. The styling is simple, as the style merely holds the properties and the widgets determines what properties from the style to apply and how. 

        There is no connection between styles and widgets, i.e. applying style to a widget, then changing the style does not change the widget. Applying a style to a widget is always explicit. 
     */
    class Style {
    public:

        static Style & defaultStyle();

        static void saveDefaultStyle();

        void load(ini::Reader & reader);

        void save(ini::Writer & writer);

        Color defaultFg() const { return defaultFg_; }
        Color defaultBg() const { return defaultBg_; }

        void setDefaultFg(Color value) { defaultFg_ = value; }
        void setDefaultBg(Color value) { defaultBg_ = value; }

        Color accentFg() const { return accentFg_; }
        Color accentBg() const { return accentBg_; }

        void setAccentFg(Color value) { accentFg_ = value; }
        void setAccentBg(Color value) { accentBg_ = value; }


        uint32_t animationSpeed() const { return animationSpeed_; }

        ImageSource const & backgroundImage() const { return backgroundImage_; }

        void setBackgroundImage(ImageSource img) {
            backgroundImage_ = std::move(img);
        }

    private:
        static constexpr char const * STYLE_SETTINGS_FILE = "style2.ini";

        Color defaultFg_ = Color::White();
        Color defaultBg_ = Color::Black();
        
        Color accentFg_ = Color::White();
        Color accentBg_ = Color::RGB(32, 32, 32);
        
        uint32_t animationSpeed_ = RCKID_DEFAULT_ANIMATION_DURATION_MS;

        ImageSource backgroundImage_{assets::images::logo};

        static inline Style * defaultStyle_ = nullptr;

    }; // rckid::ui::Style



} // namespace rckid::ui