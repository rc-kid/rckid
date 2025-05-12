#pragma once

#include <vector>

#include "../utils/buffers.h"
#include "panel.h"
#include "../app.h"

namespace rckid::ui {

    class Form : public Panel {
    public:
        explicit Form():
            Form{320,240} {
        }

        Form(Coord width, Coord height):
            Form(Rect::Centered(width, height, RCKID_DISPLAY_WIDTH, RCKID_DISPLAY_HEIGHT)) {
        }

        Form(Rect rect): 
            Panel{rect},
            buffer_{RCKID_DISPLAY_HEIGHT} {
        }

        void initialize() {
            displaySetRefreshDirection(DisplayRefreshDirection::ColumnFirst);
            displaySetUpdateRegion(rect());
        }

        void finalize() {
            // nothing to do in the finalizer
        }

        /** Renders the form on the display in a column-wise manner.
         */
        void render() {
            column_ = width() - 1;
            renderColumn(column_, buffer_.front(), 0, height());
            renderColumn(column_ - 1, buffer_.back(), 0, height());
            displayWaitVSync();
            displayUpdate(buffer_.front(), height(), [&](){
                if (--column_ < 0)
                    return;
                buffer_.swap();
                displayUpdate(buffer_.front(), height());
                if (column_ > 0)
                    renderColumn(column_ - 1, buffer_.back(), 0, height());
            });
        }

    protected:

        DoubleBuffer<uint16_t> buffer_;
        Coord column_;
    }; 

    template<typename T>
    using App = RenderableApp<ui::Form, T>;

} // namespace rckid::ui