#pragma once

#include <rckid/ui/panel.h>
#include <rckid/ui/image.h>
#include <rckid/ui/header.h>
#include <rckid/buffer.h>

namespace rckid::ui {

    class RootWidget : public Panel {
    public:

        RootWidget();

        RootWidget(Rect rect, Theme theme): 
            renderBuffer_{static_cast<uint32_t>(rect.height())},
            theme_{theme} 
        {
            setRect(rect);
            if (header_ == nullptr)
                header_ = std::make_unique<Header>();
        }

        ui::Theme theme() const override {
            return theme_;
        }

        bool useBackrgoundImage() const { return useBackgroundImage_; }

        void useBackgroundImage(bool value) {
            useBackgroundImage_ = value;
        }

        void useHeader(bool value) {
            useHeader_ = value;
            if (globalUseHeader_ != value) {
                globalUseHeader_ = value;
                if (value)
                    header_->animate()
                        << Move(header_.get(), Point{0, -20}, Point{0,0}, animationSpeed_);
                else
                    header_->animate()
                        << Move(header_.get(), Point{0,0}, Point{0, -20}, animationSpeed_);
            }
        }

        void initializeDisplay();

        void render();

        void applyStyle(Style const * style, Theme theme) override {
            theme_ = theme;
            if (style == nullptr)
                return;
            Panel::applyStyle(style, theme);
            animationSpeed_ = style->animationSpeed();
        }

        uint32_t animationSpeed() const { return animationSpeed_; }

        void setAnimationSpeed(uint32_t speed) { animationSpeed_ = speed; }

        void setBackgroundImage(Style const * style) {
            if (style->backgroundImage().empty()) {
                background_ = nullptr;
            } else {
                background_.reset(new Image());
                with(background_.get())
                    << SetBitmap(style->backgroundImage()).withoutTransparency()
                    << SetRect(Rect::XYWH(0, 0, hal::display::WIDTH, hal::display::HEIGHT))
                    << SetHAlign(HAlign::Center)
                    << SetVAlign(VAlign::Center)
                    << SetContentsRepeat(true);
                // reset background centering to manual so that it can be properly animated by the background effects
                with(background_.get())
                    << SetHAlign(HAlign::Manual)
                    << SetVAlign(VAlign::Manual);
                // and start background animation for the first 100ms so that the first carousel page transition will not trigger
                background_->animate()
                    << Move(background_.get(), background_->position(), background_->position(), 100);
            }
        }

        /** Starts background transition effect in given direction and duration.
         */
        static void backgroundEffect(Direction dir, uint32_t durationMs) {
            // do not animate when the background is not idle as otherwise we can get artefacts due to inavlid start position
            if (background_ == nullptr || ! background_->idle())
                return;
            background_->animate()
                << OffsetContents(background_.get(), background_->contentsOffset(), background_->contentsOffset() + dir * -25, durationMs)->setEasingFunction(easing::inOutIn);
        }

        /** Root widget overrides the panel's render column in order to properly render the background image and header. 
         */
        void renderColumn(Coord column, Coord startRow, Color::RGB565 * buffer, Coord numPixels) override {
            if (useBackgroundImage_ && background_ != nullptr) {
                renderChildColumn(background_.get(), column, startRow, buffer, numPixels);
            } else {
                memset16(reinterpret_cast<uint16_t*>(buffer), bg_.toRGB565(), numPixels);
            }

            Widget::renderColumn(column, startRow, buffer, numPixels);

            if (y() == 0 && (globalUseHeader_ || ! header_->idle()))
                renderChildColumn(header_.get(), column, startRow, buffer, numPixels);
        }
        
    private:

        DoubleBuffer<Color::RGB565> renderBuffer_;
        Coord renderCol_;

        Theme theme_;
        bool useBackgroundImage_ = true;
        bool useHeader_ = true;
        uint32_t animationSpeed_ = 500;

        static inline unique_ptr<Header> header_;
        static inline bool globalUseHeader_ = false;

        /** Background image (wallpaper)
         
            The image is static because it can be shared between multiple root widgets to save on RAM. Alternatively it can be removed at any time to save RAM if not needed by the current app.
         */
        static inline unique_ptr<Image> background_;

    }; // ui::RootWidget


    struct UseBackgroundImage {
        bool value;
        UseBackgroundImage(bool value = true): value{value} {}
    };
    
    template<typename T>
    inline with<T> operator << (with<T> w, UseBackgroundImage v) {
        w->useBackgroundImage(v.value);
        return w;
    }

} // namespace rckid