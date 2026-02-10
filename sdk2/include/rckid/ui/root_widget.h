#pragma once

#include <rckid/ui/panel.h>
#include <rckid/ui/image.h>
#include <rckid/buffer.h>

namespace rckid::ui {

    class RootWidget : public Panel {
    public:

        RootWidget();

        RootWidget(Rect rect): 
            renderBuffer_{static_cast<uint32_t>(rect.height())} {
            setRect(rect);
        }

        bool useBackrgoundImage() const { return useBackgroundImage_; }

        void useBackgroundImage(bool value) {
            useBackgroundImage_ = value;
        }

        void initializeDisplay();

        void render();

        void applyStyle(Style const * style) override {
            if (style == nullptr)
                return;
            Panel::applyStyle(style);
            if (style->backgroundImage().empty()) {
                background_ = nullptr;
            } else {
                background_.reset(new Image());
                with(background_.get())
                    << SetBitmap(style->backgroundImage())
                    << SetRect(Rect::XYWH(0, 0, width(), height()))
                    << SetTransparentColor(std::nullopt)
                    << SetHAlign(HAlign::Center)
                    << SetVAlign(VAlign::Center);
            }
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
        }
        
    private:

        DoubleBuffer<Color::RGB565> renderBuffer_;
        Coord renderCol_;

        bool useBackgroundImage_ = true;

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