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

        void initializeDisplay();

        void render();

        /** Root widget overrides the panel's render column in order to properly render the background image and header. 
         */
        void renderColumn(Coord column, Coord startRow, Color::RGB565 * buffer, Coord numPixels) override {
            memset16(reinterpret_cast<uint16_t*>(buffer), bg_.toRGB565(), numPixels);
            Widget::renderColumn(column, startRow, buffer, numPixels);
        }
        
    private:

        DoubleBuffer<Color::RGB565> renderBuffer_;
        Coord renderCol_;

        /** Background image (wallpaper)
         
            The image is static because it can be shared between multiple root widgets to save on RAM. Alternatively it can be removed at any time to save RAM if not needed by the current app.
         */
        static inline Image * background_ = nullptr;

    }; // ui::RootWidget

} // namespace rckid