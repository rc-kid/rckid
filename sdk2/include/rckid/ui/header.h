#pragma once

#include <rckid/ui/tile_grid.h>

namespace rckid::ui {

    class Header : public TileGrid {
    public:

        static bool shouldShow() {
            return show_ || ! instance_->idle();
        }

        static void show(bool value) {
            if (show_ != value) {
                show_ = value;
                if (value)
                    instance_->animate()
                        << Move(instance_, Point{0, -20}, Point{0,0}, instance_->animationSpeed_);
                else
                    instance_->animate()
                        << Move(instance_, Point{0,0}, Point{0, -20}, instance_->animationSpeed_);
            }
        }


        uint32_t animationSpeed() const { return animationSpeed_; }

        void setAnimationSpeed(uint32_t speed) { animationSpeed_ = speed; }

        void applyStyle(Style const * style, Theme theme) override {
            if (style == nullptr)
                return;
            TileGrid::applyStyle(style, theme);
            animationSpeed_ = style->animationSpeed();
        }

    private:

        friend class RootWidget;

        Header():
            TileGrid{display::WIDTH / TileGrid::tileWidth(), 1, defaultPalette()}
        {
            setRect(Rect::WH(display::WIDTH, TileGrid::tileHeight()));
            contents_.text(0,0) << "RCKid SDK 1.0";
        }

        static mutable_ptr<Color::RGB565> defaultPalette() {
            mutable_ptr<Color::RGB565> result{new Color::RGB565[32], sizeof(Color::RGB565) * 32};
            Color textFg = Style::defaultStyle()->defaultFg();
            for (uint8_t i = 0; i < 16; ++i)
                result.mut()[i] = textFg.withBrightness(i << 4 | i).toRGB565();
            return result;
        }

        uint32_t animationSpeed_ = 500;

        static inline bool show_ = false;
        static inline Header * instance_ = nullptr;

    }; // rckid::ui::Header

} // namespace rckid::ui