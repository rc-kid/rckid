#pragma once

#include <rckid/ui/tile_grid.h>

namespace rckid::ui {

    /** UI Notification area. 
     
        Displays notifications about the system state. Supports both column & row based rendering so that it can be used also in non UI-based applications. The header is a singleton widget to which the applications do not have direct access. 

        The header can either be always visible, or only show up when there is some change. 
     */
    class Header : public TileGrid {
    public:

        enum class Visibility {
            Always,
            OnChange,
        }; 

        /** Returns true if the header should be rendered in the current frame.
         */
        static bool shouldRender() {
            return instance_->visible();
        }

        static void setVisibility(Visibility visibility) {
            switch (visibility) {
                // if the header is to be always visible,
                case Visibility::Always:
                    instance_->remainingTicks_ = TicksToShowOnChange + 1;
                    instance_->show();
                    break;
                case Visibility::OnChange:
                    if (instance_->visible()) {
                        if (instance_->remainingTicks_ > TicksToShowOnChange)
                            instance_->remainingTicks_ = 1;
                    }
                    break;
            }
        }

        /** Updates the header icons
         */
        static void update();

        /** Palette offsets for different colors. 
         */
        static constexpr uint8_t PaletteOffsetGreen = 14;
        static constexpr uint8_t PaletteOffsetRed = 16;
        static constexpr uint8_t PaletteOffsetBlue = 18;
        static constexpr uint8_t PaletteOffsetCyan = 20;
        static constexpr uint8_t PaletteOffsetViolet = 22;


        void renderRow(Coord row, Coord startCol, Color::RGB565 * buffer, Coord numPixels) {
            UNIMPLEMENTED;
        }

    protected:

        static constexpr int32_t TicksToShowOnChange = 60 * 5; // show for 5 seconds after change
      
        /** When rendering, determine if we should  */
        void onRender() override {
            if (remainingTicks_ <= TicksToShowOnChange && remainingTicks_ > 0) {
                if (--remainingTicks_ == 0)
                    hide();
            }
        }

        void onIdle() override {
            if (remainingTicks_ == 0)
                Widget::setVisibility(false);
        }

    private:

        friend class RootWidget;

        Header():
            TileGrid{display::WIDTH / TileGrid::tileWidth(), 1, defaultPalette()}
        {
            ASSERT(instance_ == nullptr);
            instance_ = this;
            setRect(Rect::XYWH(0, - TileGrid::tileHeight(), display::WIDTH, TileGrid::tileHeight()));
            Widget::setVisibility(false);
            update();
        }

        void show() {
            if (visible())
                return;
            Widget::setVisibility(true);
            animate()
                << MoveTo(this, Point{0,0});
        }

        void hide() {
            if (! visible())
                return;
            animate()
                << MoveTo(this, Point{0, -TileGrid::tileHeight()});
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


        /** Ticks remaining for the header to be shown. 
        
         */
        int32_t remainingTicks_ = 0;

        static inline Header * instance_ = nullptr;

    }; // rckid::ui::Header

} // namespace rckid::ui