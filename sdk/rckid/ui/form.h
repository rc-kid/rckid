#pragma once

#include <vector>

#include "../utils/buffers.h"
#include "panel.h"

namespace rckid::ui {

    class Form : public Panel {
    public:
        Form(Allocator & a = Heap::allocator()) {
            setRect(Rect::WH(320, 240));
            buffer_ = new (a.alloc<DoubleBuffer<uint16_t>>()) DoubleBuffer<uint16_t>(240, a);
        }

        ~Form() override {
            Heap::tryFree(buffer_);
        }

        void initialize() {
            initialize(DisplayResolution::Full);
        }

        void initialize(DisplayResolution res) {
            displaySetResolution(res);
            displaySetRefreshDirection(DisplayRefreshDirection::Native);
            displaySetUpdateRegion(rect());
        }

        void finalize() {
            // nothing to do in the finalizer
        }

        void render() {
            column_ = width() - 1;
            renderColumn(column_, buffer_->front(), 0, 240);
            renderColumn(column_ - 1, buffer_->back(), 0, 240);
            displayWaitVSync();
            displayUpdate(buffer_->front(), 240, [&]() {
                if (--column_ < 0)
                    return;
                buffer_->swap();
                displayUpdate(buffer_->front(), 240);
                if (column_ > 0)
                    renderColumn(column_ - 1, buffer_->back(), 0, 240);
            });
        }

    protected:

        DoubleBuffer<uint16_t> * buffer_;
        Coord column_;

    }; 

} // namespace rckid::ui