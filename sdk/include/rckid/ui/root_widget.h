#pragma once

#include <rckid/ui/panel.h>
#include <rckid/ui/image.h>
#include <rckid/ui/header.h>
#include <rckid/buffer.h>

namespace rckid::ui {

    class RootWidget : public Panel {
    public:

        RootWidget();

        RootWidget(Rect rect): 
            renderBuffer_{static_cast<uint32_t>(rect.height())}
        {
            setRect(rect);
            if (Header::instance_ == nullptr) {
                Header::instance_ = new Header{};
                Header::instance_->applyStyle(Style::defaultStyle());
            }
        }

        bool useBackrgoundImage() const { return useBackgroundImage_; }

        void useBackgroundImage(bool value) {
            useBackgroundImage_ = value;
        }

        Header::Visibility useHeader() const { return useHeader_; }

        void setUseHeader(Header::Visibility value) {
            if (value == useHeader_)
                return;
            useHeader_ = value;
            Header::setVisibility(value);
        }

        void initializeDisplay();

        void render();

        void setBackgroundImage(Style const & style) {
            if (style.backgroundImage().empty()) {
                background_ = nullptr;
            } else {
                background_.reset(new Image());
                with(background_.get())
                    << SetBitmap(style.backgroundImage()).withoutTransparency()
                    << SetRect(Rect::XYWH(0, 0, hal::display::WIDTH, hal::display::HEIGHT))
                    << SetHAlign(HAlign::Center)
                    << SetVAlign(VAlign::Center)
                    << SetContentsRepeat(true);
                // if for any reason loading the image failed, delete the background
                if (background_->contents().empty()) {
                    background_ = nullptr;
                    return;
                }
                // reset background centering to manual so that it can be properly animated by the background effects
                with(background_.get())
                    << SetHAlign(HAlign::Manual)
                    << SetVAlign(VAlign::Manual);
                // and start background animation for the first 100ms so that the first carousel page transition will not trigger
                background_->animate()
                    << Move(background_.get(), background_->position(), background_->position());
            }
        }

        /** Starts background transition effect in given direction and duration.
         */
        static void backgroundEffect(Direction dir, uint32_t durationMs) {
            // do not animate when the background is not idle as otherwise we can get artefacts due to inavlid start position
            if (background_ == nullptr || ! background_->idle())
                return;
            background_->animate()
                << OffsetContents(background_.get(), background_->contentsOffset(), background_->contentsOffset() + dir * -25)->setEasingFunction(easing::inOutIn)->setDurationMs(durationMs);
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

            if (y() == 0 && Header::shouldRender())
                renderChildColumn(Header::instance_, column, startRow, buffer, numPixels);
        }

        /** Releases the resources helpd by the root widget.
         */
        static void releaseResources() {
            background_ = nullptr;
        }
      
    private:

        DoubleBuffer<Color::RGB565> renderBuffer_;
        Coord renderCol_;

        bool useBackgroundImage_ = true;
        Header::Visibility useHeader_ = ui::Header::Visibility::Always;

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