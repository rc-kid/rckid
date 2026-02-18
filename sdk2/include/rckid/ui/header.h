#pragma once

#include <rckid/ui/tile_grid.h>

namespace rckid::ui {

    class Header : public TileGrid {
    public:

        static constexpr uint8_t PaletteOffsetGreen = 14;

        static bool shouldShow() {
            return show_ || ! instance_->idle();
        }

        static void show(bool value) {
            if (show_ != value) {
                show_ = value;
                if (value)
                    instance_->animate()
                        << Move(instance_, Point{0, -20}, Point{0,0});
                else
                    instance_->animate()
                        << Move(instance_, Point{0,0}, Point{0, -20});
            }
        }


        uint32_t animationSpeed() const { return animationSpeed_; }

        void setAnimationSpeed(uint32_t speed) { animationSpeed_ = speed; }

    protected:

        void doApplyStyle(Style const & style, Theme theme) override {
            TileGrid::doApplyStyle(style, theme);
            animationSpeed_ = style.animationSpeed();
        }

    private:

        friend class RootWidget;

        Header():
            TileGrid{display::WIDTH / TileGrid::tileWidth(), 1, defaultPalette()}
        {
            setRect(Rect::WH(display::WIDTH, TileGrid::tileHeight()));
            contents_.text(0,0) << "RCKid SDK 1.0";
            contents_.setTileIcon(37, 0, TileIcon::batteryFull(), PaletteOffsetGreen);
        }

        static mutable_ptr<Color::RGB565> defaultPalette() {
            mutable_ptr<Color::RGB565> result{new Color::RGB565[32], sizeof(Color::RGB565) * 32};
            Color textFg = Style::defaultStyle()->defaultFg();
            for (uint8_t i = 0; i < 16; ++i)
                result.mut()[i] = textFg.withBrightness(i << 4 | i).toRGB565();
            // green (testFg before that used for index 1, offset 14)
            result.mut()[16] = Color::RGB(0, 255, 0).toRGB565();
            // red (offset 16)
            result.mut()[17] = textFg.toRGB565();
            result.mut()[18] = Color::RGB(255, 0, 0).toRGB565();
            // blue (offset 18)
            result.mut()[19] = textFg.toRGB565();
            result.mut()[20] = Color::RGB(0, 0, 255).toRGB565();
            // cyan (offset 20) 
            result.mut()[21] = textFg.toRGB565();
            result.mut()[22] = Color::RGB(0, 255, 255).toRGB565();
            // violet (ioffset 22) 
            result.mut()[23] = textFg.toRGB565();
            result.mut()[24] = Color::RGB(255, 0, 255).toRGB565();
           
            return result;
        }

        uint32_t animationSpeed_ = 500;

        static inline bool show_ = false;
        static inline Header * instance_ = nullptr;

    }; // rckid::ui::Header

} // namespace rckid::ui