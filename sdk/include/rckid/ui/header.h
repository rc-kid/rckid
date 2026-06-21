#pragma once

#include <rckid/ui/tile_grid.h>

namespace rckid::ui {

    /** UI Notification area. 
     
        Displays notifications about the system state. Supports both column & row based rendering so that it can be used also in non UI-based applications. As the header is tiny, it is internally a singleton object to which the applications do not have direct access. 

        The header can either be always visible, or only show up when there is some change. 
     */
    class Header : public TileGrid {
    public:

        static constexpr uint32_t ON_CHANGE_VISIBILITY_TICKS = 60 * 5;

        enum class Visibility {
            Always, 
            OnChange,  
            Never,
        }; 

        /** Returns the current visibility of the header. 
         */
        static bool currentVisibility() {
            return instance()->visible();
        }

        /** Returns the visibility settings for the header.
         */
        static Visibility visibility() { return visibility_; }

        /** Sets the visibility settings for the header. 
         */
        static void setVisibility(Visibility visibility) {
            // don't do anything if the visibility is the same
            if (visibility_ == visibility)
                return;
            Visibility old = visibility_;
            visibility_ = visibility;
            instance()->updateVisibility(old);
        }

        /** Returns true if the header should be rendered in the current frame.
         */
        static bool shouldRender() {
            return instance()->visible();
        }

        /** Updates the header icons
         */
        static void update();


        static Header * instance() { 
            if (instance_ == nullptr) {
                instance_ = new Header{};
                instance_->applyStyle(Style::defaultStyle());
            }
            return instance_; 
        }

        /** Palette offsets for different colors. 
         */
        static constexpr uint8_t PaletteOffsetGreen = 14;
        static constexpr uint8_t PaletteOffsetRed = 16;
        static constexpr uint8_t PaletteOffsetBlue = 18;
        static constexpr uint8_t PaletteOffsetCyan = 20;
        static constexpr uint8_t PaletteOffsetViolet = 22;



        void renderRow(Coord row, Coord startCol, Color::RGB565 * buffer, Coord numPixels) {
            Coord ownRow = row - rect().y;
            if (ownRow < 0 || ownRow >= height())
                return;
            // adjust numPixels if we exceed contents width
            if (startCol + numPixels > width())      
                numPixels = width() - startCol;
            if (numPixels <= 0)
                return;
            contents().renderRow(ownRow, startCol, buffer, numPixels);
        }

    protected:


        /** When rendering, determine if we should  */
        void onRender() override {
            if (visibility_ == Visibility::OnChange)
                if ((remainingTicks_ > 0) && (--remainingTicks_ == 0))
                    hide();
        }

        void onIdle() override {
            if (remainingTicks_ == 0) {
                Widget::setVisibility(false);
                setRect(Rect::XYWH(0, - TileGrid::tileHeight(), width(), height()));
            }
        }

    private:

        Header():
            TileGrid{display::WIDTH / TileGrid::tileWidth(), 1, nullptr},
            palette_{defaultPalette()}
        {
            ASSERT(instance_ == nullptr);
            contents().setPalette(palette_.get());
            instance_ = this;
            setRect(Rect::XYWH(0, - TileGrid::tileHeight(), display::WIDTH, TileGrid::tileHeight()));
            Widget::setVisibility(false);
            update();
        }

        void updateVisibility(Visibility old) {
            switch (visibility_) {
                case Visibility::Always:
                    show(); // calling this before the remainingTicks set so that if in hide animation in progress, we will interrupt
                    remainingTicks_ = ON_CHANGE_VISIBILITY_TICKS + 1;
                    break;
                case Visibility::OnChange:
                    if (old == Visibility::Always)
                        hide();
                    if (remainingTicks_ > ON_CHANGE_VISIBILITY_TICKS)
                        remainingTicks_ = ON_CHANGE_VISIBILITY_TICKS;
                    // no need to show (change does that)
                    break;
                case Visibility::Never:
                    remainingTicks_ = 0;
                    hide();
                    break;
                default:
                    UNREACHABLE;
            }
        }

        /** Ensures the header is visible, bringing it into view when necessary.
         */
        void show() {
            // don't show when app requests no header at all
            if (visibility_ == Visibility::Never)
                return;
            // if there are no animations in process and the header is visible, there is nothing to do
            if (idle() && visible()) {
                if (remainingTicks_ < ON_CHANGE_VISIBILITY_TICKS)
                    remainingTicks_ = ON_CHANGE_VISIBILITY_TICKS;
                return;
            }
            // if the current animation is *showing*, there is nothing to do, we can tell by ticks being non-zero
            if (remainingTicks_ > 0) {
                if (remainingTicks_ < ON_CHANGE_VISIBILITY_TICKS)
                    remainingTicks_ = ON_CHANGE_VISIBILITY_TICKS;
                return;
            }
            // otherwise cancel any pending animations and start the show animation
            cancelAnimations();
            Widget::setVisibility(true);
            animate()
                << MoveTo(this, Point{0,0});
            // reset the hide countdown
            remainingTicks_ = ON_CHANGE_VISIBILITY_TICKS;
        }

        /** Hides the header, if this is allowed by the setti  */
        void hide() {
            // nothing to do when visibility is set to always
            if (visibility_ == Visibility::Always)
                return;
            // if not visible already nothing to do
            if (! visible()) {
                setRect(Rect::XYWH(0, - TileGrid::tileHeight(), width(), height()));
                return;
            }
            animate()
                << MoveTo(this, Point{0, -TileGrid::tileHeight()});
            remainingTicks_ = 0;
        }

        static immutable_ptr<Color::RGB565> defaultPalette() {
            Color::RGB565 * p = new Color::RGB565[32];
            Color textFg = Style::defaultStyle().defaultFg();
            for (uint8_t i = 0; i < 16; ++i)
                p[i] = textFg.withBrightness(i << 4 | i).toRGB565();
            // green (testFg before that used for index 1, offset 14)
            p[16] = Color::RGB(0, 255, 0).toRGB565();
            // red (offset 16)
            p[17] = textFg.toRGB565();
            p[18] = Color::RGB(255, 0, 0).toRGB565();
            // blue (offset 18)
            p[19] = textFg.toRGB565();
            p[20] = Color::RGB(0, 0, 255).toRGB565();
            // cyan (offset 20) 
            p[21] = textFg.toRGB565();
            p[22] = Color::RGB(0, 255, 255).toRGB565();
            // violet (ioffset 22) 
            p[23] = textFg.toRGB565();
            p[24] = Color::RGB(255, 0, 255).toRGB565();
           
            return immutable_ptr<Color::RGB565>{p, 32};
        }


        /** Ticks remaining for the header to be shown. 
        
         */
        uint32_t remainingTicks_ = 0;

        immutable_ptr<Color::RGB565> palette_;
        
        static inline Visibility visibility_ = Visibility::Always;

        static inline Header * instance_ = nullptr;

    }; // rckid::ui::Header

} // namespace rckid::ui