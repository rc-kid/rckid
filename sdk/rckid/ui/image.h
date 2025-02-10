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
            reposition();
        }

        HAlign hAlign() const { return hAlign_; }
        VAlign vAlign() const { return vAlign_; }
        bool repeat() const { return repeat_; }
        
        void setHAlign(HAlign value) {
            if (value != hAlign_) {
                hAlign_ = value;
                reposition();
            }
        }

        void setVAlign(VAlign value) {
            if (value != vAlign_) {
                vAlign_ = value;
                reposition();
            }
        }

        void renderColumn(Coord column, Pixel * buffer, Coord starty, Coord numPixels) override {
            // the image should not be repeated, translate the actual bitmap's rectangle as if it were a child widget and then use its column rendering
            if (!repeat_) {
                adjustRenderParams(Rect::XYWH(imgX_, imgY_, bmp_.width(), bmp_.height()), column, buffer, starty, numPixels);
                if (numPixels != 0)  
                    bmp_.renderColumn(column, buffer, starty, numPixels);
            // if repeated, adjust column to always fit into the image, figure out starty and then draw the column over and over the entire height
            } else {
                column = std::abs(column - imgX_) % bmp_.width();
                starty = std::abs(starty - imgY_) % bmp_.height();
                while (numPixels > 0) {
                    Coord n = std::min(numPixels, bmp_.height() - starty);
                    bmp_.renderColumn(column, buffer, starty, n);
                    buffer += n;
                    numPixels -= n;
                    starty = 0;
                }
            }
        }

    protected:
        void reposition() {
            switch (hAlign_) {
                case HAlign::Left:
                    imgX_ = 0;
                    break;
                case HAlign::Center:
                    imgX_ = (width() - bmp_.width()) / 2;
                    break;
                case HAlign::Right:
                    imgX_ = width() - bmp_.width();
                    break;
            }
            switch (vAlign_) {
                case VAlign::Top:
                    imgY_ = 0;
                    break;
                case VAlign::Center:
                    imgY_ = (height() - bmp_.height()) / 2;
                    break;
                case VAlign::Bottom:
                    imgY_ = height() - bmp_.height();
                    break;
            }
        }

        void resize() override {
            reposition();
        }

    private:
        Bitmap<BPP> bmp_;

        HAlign hAlign_ = HAlign::Center;
        VAlign vAlign_ = VAlign::Center;
        bool repeat_ = false;
        Coord imgX_;
        Coord imgY_; 

    }; // rckid::ui::Image

} // namespace rckid::ui