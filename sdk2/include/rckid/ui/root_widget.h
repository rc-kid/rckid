#pragma once

#include <rckid/ui/panel.h>
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
        
    private:

        DoubleBuffer<Color::RGB565> renderBuffer_;
        Coord renderCol_;

    }; // ui::RootWidget

} // namespace rckid