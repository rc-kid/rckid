#pragma once

#include "../graphics/bitmap.h"
#include "widget.h"

namespace rckid::ui {

    /** A simple image holder widget. 
     */
    class Image : public Widget {
    public:

        /** Creates an image with empty 16bpp bitmap. 
         */
        Image():
            bmp_{} {
        }

        Image(Rect rect, Bitmap<ColorRGB> && bmp):
            Widget{rect},
            bmp_{std::move(bmp)} {
            reposition();
        }
    
        Image(Bitmap<ColorRGB> && bmp): bmp_{std::move(bmp)} {
            w_ = bmp_.width();
            h_ = bmp_.height();
            reposition();
        }

        Image(Image const &) = delete;

        Image(Image && other) noexcept {
            assign(std::move(other));
        }

        Image & operator = (Image && other) noexcept {
            if (this != & other) {
                Widget::operator=(other);
                assign(std::move(other));
            }
            return *this;
        }

        Image & operator = (Bitmap<ColorRGB> && bmp) noexcept {
            setBitmap(std::move(bmp));
            return *this;
        }

        void setBitmap(Bitmap<ColorRGB> && bmp) noexcept {
            bmp_.~Bitmap();
            new (&bmp_) Bitmap<ColorRGB>{std::move(bmp)};
            w_ = bmp_.width();
            h_ = bmp_.height();
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
            return;
        }

        void setVAlign(VAlign value) {
            if (value != vAlign_) {
                vAlign_ = value;
                reposition();
            }
        }

        void setRepeat(bool value) {
            repeat_ = value;
        }

        Coord imgX() const { return imgX_; }
        Coord imgY() const { return imgY_; }
        void setImgX(Coord value) { imgX_ = value % bitmapWidth(); }

        void setImgY(Coord value) { imgY_ = value % bitmapHeight(); }

        bool transparent() const { return transparent_ <= 0xffff; }
        void setTransparent(bool value) { transparent_ = value ? 0 : NO_TRANSPARENCY; }
        ColorRGB transparentColor() const { return ColorRGB::fromRaw(transparent_); }
        void setTransparentColor(ColorRGB color) { transparent_ = color.toRaw(); }

        bool empty() { return bmp_.empty(); }

        void clear() {
            bmp_.clear();
        }

    protected:

        void renderColumn(Coord column, uint16_t * buffer, Coord starty, Coord numPixels) override {
            // the image should not be repeated, translate the actual bitmap's rectangle as if it were a child widget and then use its column rendering
            if (!repeat_) {
                adjustRenderParams(Rect::XYWH(imgX_, imgY_, bitmapWidth(), bitmapHeight()), column, buffer, starty, numPixels);
                if (numPixels != 0)  
                    renderBitmapColumn(column, buffer, starty, numPixels);
            // if repeated, adjust column to always fit into the image, figure out starty and then draw the column over and over the entire height
            } else {
                column = (column - imgX_) % bitmapWidth();
                starty = (starty - imgY_) % bitmapHeight();
                if (column < 0)
                    column = bitmapWidth() + column;
                if (starty < 0)
                    starty = bitmapHeight() + starty;
                while (numPixels > 0) {
                    Coord n = std::min(numPixels, bitmapHeight() - starty);
                    renderBitmapColumn(column, buffer, starty, n);
                    buffer += n;
                    numPixels -= n;
                    starty = 0;
                }
            }
        }

        void resize() override {
            reposition();
        }

        void reposition() {
            Point x = justifyRectangle(Rect::WH(bitmapWidth(), bitmapHeight()), hAlign_, vAlign_);
            imgX_ = x.x;
            imgY_ = x.y;
        }

    private:

        void assign(Image && other) {
            bmp_ = std::move(other.bmp_);
            hAlign_ = other.hAlign_;
            vAlign_ = other.vAlign_;
            repeat_ = other.repeat_;
            imgX_ = other.imgX_;
            imgY_ = other.imgY_;
            transparent_ = other.transparent_;
        }

        void renderBitmapColumn(Coord column, uint16_t * buffer, Coord starty, Coord numPixels) {
            if (transparent_ < 0xffff)
                bmp_.renderColumn(column, starty, numPixels, buffer, transparent_);
            else 
                bmp_.renderColumn(column, starty, numPixels, buffer);
        }

        Coord bitmapWidth() const {
            return bmp_.width();
        }

        Coord bitmapHeight() const {
            return bmp_.height();
        }

        Bitmap<ColorRGB> bmp_;

        HAlign hAlign_ = HAlign::Center;
        VAlign vAlign_ = VAlign::Center;
        bool repeat_ = false;
        Coord imgX_;
        Coord imgY_; 

        static constexpr uint32_t NO_TRANSPARENCY = 0xffffffff;
        uint32_t transparent_ = NO_TRANSPARENCY;

    }; // rckid::ui::Image

} // namespace rckid::ui