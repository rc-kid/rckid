#pragma once

#include <vector>

#include "../utils/buffers.h"
#include "panel.h"

namespace rckid::ui {

    class Form : public Panel {
    public:
        Form(Coord width, Coord height, Allocator & a = Heap::allocator()):
            Form(Rect::Centered(width, height, RCKID_DISPLAY_WIDTH, RCKID_DISPLAY_HEIGHT), a) {
        }

        Form(Rect rect, Allocator & a = Heap::allocator()): Panel{rect} {
            buffer_ = new (a.alloc<DoubleBuffer<uint16_t>>()) DoubleBuffer<uint16_t>(RCKID_DISPLAY_HEIGHT);
        }

        ~Form() override {
            Heap::tryFree(buffer_);
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
            renderColumn(column_, buffer_->front(), 0, height());
            renderColumn(column_ - 1, buffer_->back(), 0, height());
            displayWaitVSync();
            displayUpdate(buffer_->front(), height(), [&]() {
                if (--column_ < 0)
                    return;
                buffer_->swap();
                displayUpdate(buffer_->front(), height());
                if (column_ > 0)
                    renderColumn(column_ - 1, buffer_->back(), 0, height());
            });
        }

    protected:

        DoubleBuffer<uint16_t> * buffer_;
        Coord column_;
    }; 

} // namespace rckid::ui