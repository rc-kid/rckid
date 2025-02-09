#pragma once

#include "../graphics/bitmap.h"
#include "widget.h"

namespace rckid::ui {

    /** A simple image holder widget. 
     */
    template<uint32_t BPP>
    class Image : public Widget {
    public:
        
        Image(Bitmap<BPP> && bmp): bmp_{std::move(bmp)} {
        }

        void renderColumn(Coord column, Pixel * buffer) override {
            UNIMPLEMENTED;
        }

    private:
        Bitmap<BPP> bmp_;

    }; // rckid::ui::Image

} // namespace rckid::ui