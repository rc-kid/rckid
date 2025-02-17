#pragma once

#include "../graphics/bitmap.h"
#include "widget.h"

namespace rckid::ui {

    /** A simple image holder widget. 
     */
    class Image : public Widget {
    public:

        Image():
            bmp2_{}, bpp_{2} {
        }
    
        Image(Bitmap<2> && bmp): bmp2_{std::move(bmp)}, bpp_{2} {
            w_ = bmp2_.width();
            h_ = bmp2_.height();
            reposition();
        }

        Image(Bitmap<4> && bmp): bmp4_{std::move(bmp)}, bpp_{4} {
            w_ = bmp4_.width();
            h_ = bmp4_.height();
            reposition();
        }
        Image(Bitmap<8> && bmp): bmp8_{std::move(bmp)}, bpp_{8} {
            w_ = bmp8_.width();
            h_ = bmp8_.height();
            reposition();
        }
        Image(Bitmap<16> && bmp): bmp16_{std::move(bmp)}, bpp_{16} {
            w_ = bmp16_.width();
            h_ = bmp16_.height();
            reposition();
        }

        Image(Image const &) = delete;

        Image(Image && other) {
            assign(std::move(other));
        }

        Image & operator = (Image && other) {
            if (this != & other) {
                Widget::operator=(other);
                clear();
                assign(std::move(other));
            }
            return *this;
        }

        ~Image() override {
            clear();
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

    protected:

        void renderColumn(Coord column, Pixel * buffer, Coord starty, Coord numPixels) override {
            // the image should not be repeated, translate the actual bitmap's rectangle as if it were a child widget and then use its column rendering
            if (!repeat_) {
                adjustRenderParams(Rect::XYWH(imgX_, imgY_, bitmapWidth(), bitmapHeight()), column, buffer, starty, numPixels);
                if (numPixels != 0)  
                    renderBitmapColumn(column, buffer, starty, numPixels);
            // if repeated, adjust column to always fit into the image, figure out starty and then draw the column over and over the entire height
            } else {
                column = std::abs(column - imgX_) % bitmapWidth();
                starty = std::abs(starty - imgY_) % bitmapHeight();
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
            switch (other.bpp_) {
                case 2:
                    bmp2_ = std::move(other.bmp2_);
                    break;
                case 4:
                    bmp4_ = std::move(other.bmp4_);
                    break;
                case 8:
                    bmp8_ = std::move(other.bmp8_);
                    break;
                case 16:
                    bmp16_ = std::move(other.bmp16_);
                    break;
                default:
                    UNREACHABLE;
            }
            bpp_ = other.bpp_;
            hAlign_ = other.hAlign_;
            vAlign_ = other.vAlign_;
            repeat_ = other.repeat_;
            imgX_ = other.imgX_;
            imgY_ = other.imgY_;
        }

        void clear() {
            switch (bpp_) {
                case 2:
                    bmp2_.~Bitmap();
                    break;
                case 4:
                    bmp4_.~Bitmap();
                    break;
                case 8:
                    bmp8_.~Bitmap();
                    break;
                case 16:
                    bmp16_.~Bitmap();
                    break;
                default:
                    UNREACHABLE;
            }
        }

        void renderBitmapColumn(Coord column, Pixel * buffer, Coord starty, Coord numPixels) {
            switch (bpp_) {
                case 2:
                    return bmp2_.renderColumn(column, buffer, starty, numPixels);
                case 4:
                    return bmp4_.renderColumn(column, buffer, starty, numPixels);
                case 8:
                    return bmp8_.renderColumn(column, buffer, starty, numPixels);
                case 16:
                    return bmp16_.renderColumn(column, buffer, starty, numPixels);
                default:
                    UNREACHABLE;
            }
        }

        Coord bitmapWidth() const {
            switch (bpp_) {
                case 2:
                    return bmp2_.width();
                case 4:
                    return bmp4_.width();
                case 8:
                    return bmp8_.width();
                case 16:
                    return bmp16_.width();
                default:
                    UNREACHABLE;
            }
        }

        Coord bitmapHeight() const {
            switch (bpp_) {
                case 2:
                    return bmp2_.height();
                case 4:
                    return bmp4_.height();
                case 8:
                    return bmp8_.height();
                case 16:
                    return bmp16_.height();
                default:
                    UNREACHABLE;
            }
        }

        union {
            Bitmap<2> bmp2_;
            Bitmap<4> bmp4_;
            Bitmap<8> bmp8_;
            Bitmap<16> bmp16_;
        };
        uint8_t bpp_;

        HAlign hAlign_ = HAlign::Center;
        VAlign vAlign_ = VAlign::Center;
        bool repeat_ = false;
        Coord imgX_;
        Coord imgY_; 


    }; // rckid::ui::Image

} // namespace rckid::ui