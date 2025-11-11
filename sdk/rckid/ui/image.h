#pragma once

#include "../graphics/bitmap.h"
#include "../graphics/icon.h"
#include "widget.h"

namespace rckid::ui {

    class CustomImage : public Widget {
    public:

        bool empty() const { return bmp_ == nullptr || bmp_->empty(); }


        void shrinkToFit() {
            if (empty()) {
                w_ = 0;
                h_ = 0;
            } else {
                w_ = bmp_->width();
                h_ = bmp_->height();
            }
            reposition();
        }

        HAlign hAlign() const { return hAlign_; }
        VAlign vAlign() const { return vAlign_; }
        
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

        bool repeat() const { return repeat_; }
        void setRepeat(bool value) { repeat_ = value; }

        Coord imgX() const { return imgX_; }
        Coord imgY() const { return imgY_; }
        void setImgX(Coord value) { imgX_ = value % (bmp_ != nullptr ? bmp_->width() : 1); }
        void setImgY(Coord value) { imgY_ = value % (bmp_ != nullptr ? bmp_->height() : 1); }

        bool transparent() const { return transparent_ <= 0xffff; }
        void setTransparent(bool value) { transparent_ = value ? 0 : NO_TRANSPARENCY; }
        ColorRGB transparentColor() const { return ColorRGB::fromRaw(transparent_); }
        void setTransparentColor(ColorRGB color) { transparent_ = color.toRaw(); }

        void reposition() {
            if (bmp_ == nullptr)
                return;
            Point x = justifyRectangle(Rect::WH(bmp_->width() , bmp_->height()), hAlign_, vAlign_);
            imgX_ = x.x;
            imgY_ = x.y;
        }

        Bitmap const * bitmap() const { return bmp_; }
    
        void renderColumn(Coord column, uint16_t * buffer, Coord starty, Coord numPixels) override {
            if (bmp_ == nullptr)
                return;
            // the image should not be repeated, translate the actual bitmap's rectangle as if it were a child widget and then use its column rendering
            if (!repeat_) {
                adjustRenderParams(Rect::XYWH(imgX_, imgY_, bmp_->width(), bmp_->height()), column, buffer, starty, numPixels);
                if (numPixels != 0)  
                    renderBitmapColumn(column, buffer, starty, numPixels);
            // if repeated, adjust column to always fit into the image, figure out starty and then draw the column over and over the entire height
            } else {
                column = (column - imgX_) % bmp_->width();
                starty = (starty - imgY_) % bmp_->height();
                if (column < 0)
                    column = bmp_->width() + column;
                if (starty < 0)
                    starty = bmp_->height() + starty;
                while (numPixels > 0) {
                    Coord n = std::min(numPixels, bmp_->height() - starty);
                    renderBitmapColumn(column, buffer, starty, n);
                    buffer += n;
                    numPixels -= n;
                    starty = 0;
                }
            }
        }

    protected:

        CustomImage(Bitmap * bmp): bmp_{bmp} {
            if (bmp_ != nullptr) {
                w_ = bmp_->width();
                h_ = bmp_->height();
                reposition();
            }
        }

        CustomImage(Rect rect, Bitmap * bmp): 
            Widget{rect},
            bmp_{bmp} {
            if (bmp_ != nullptr)
                reposition();
        }   

        CustomImage(CustomImage const & other) = default;

        CustomImage & operator = (CustomImage const & other) = default;

        void resize() override {
            reposition();
        }

    protected:

        Bitmap * bmp_;

    private:

        void renderBitmapColumn(Coord column, uint16_t * buffer, Coord starty, Coord numPixels) {
            if (transparent_ < 0xffff)
                bmp_->renderColumn(column, starty, numPixels, buffer, transparent_);
            else 
                bmp_->renderColumn(column, starty, numPixels, buffer);
        }

        HAlign hAlign_ = HAlign::Center;
        VAlign vAlign_ = VAlign::Center;
        bool repeat_ = false;
        Coord imgX_;
        Coord imgY_; 

        static constexpr uint32_t NO_TRANSPARENCY = 0xffffffff;
        uint32_t transparent_ = 0; // black

    }; // rckid::ui::CustomImage

    /** Image that owns the bitmap. 
     
        Simple image that also manages the bitmap it displays. Useful when the bitmap is only used for a single image.
     */
    class Image : public CustomImage {
    public:
        Image() : CustomImage{nullptr} {}
        Image(Rect rect, Bitmap && bmp): CustomImage{rect, new Bitmap{std::move(bmp)}} {}
        Image(Rect rect, Icon const & icon): Image{rect, icon.toBitmap()} {}
        Image(Bitmap && bmp): CustomImage{new Bitmap{std::move(bmp)}} {}
        Image(Icon const & icon): Image{icon.toBitmap()} {}
        Image(Coord x, Coord y, Icon const & icon):
            Image{icon.toBitmap()} {
            setPos(x, y);
        }
        Image(Point pos, Icon const & icon): Image{pos.x, pos.y, icon} {}

        Image(Image && other):
            CustomImage{other} {
            other.bmp_ = nullptr;
        }

        ~Image() override {
            delete bmp_;
        }

        Image & operator = (Image && other) noexcept {
            if (this != & other) {
                CustomImage::operator=(other);
                other.bmp_ = nullptr;
            }
            return *this;
        }

        Image & operator = (Bitmap && bmp) noexcept {
            setBitmap(std::move(bmp));
            return *this;
        }

        Image & operator = (Icon const & icon) noexcept {
            if (bmp_ == nullptr)
                bmp_ = new Bitmap{icon.toBitmap()};
            else
                icon.intoBitmap(*bmp_);
            shrinkToFit();
            return *this;
        }

        void setBitmap(Bitmap && bmp) noexcept {
            delete bmp_;
            bmp_ = new Bitmap{std::move(bmp)};
            shrinkToFit();
        }

        void clear() {
            delete bmp_;
            bmp_ = nullptr;
        }

    private:

    }; // rckid::ui::Image

    /** Shared image does not own the bitmap it displays. 
     
        Useful when same bitmap is to be displayed multiple times, for example game pieces, etc. 
     */
    class SharedImage : public CustomImage {
    public:
        SharedImage() : CustomImage{nullptr} {}
        SharedImage(Rect rect, Bitmap * bmp): CustomImage{rect, bmp} {}
        SharedImage(Bitmap * bmp): CustomImage{bmp} {}
        SharedImage(Coord x, Coord y, Bitmap * bmp):
            CustomImage{Rect::XYWH(x, y, 0, 0), bmp} {
            shrinkToFit();
        }

        SharedImage(Point pos, Bitmap * bmp): SharedImage{pos.x, pos.y, bmp} {}

        SharedImage(SharedImage const & other) = default;

        SharedImage & operator = (SharedImage const & other) = default;

        void clear() {
            bmp_ = nullptr;
        }

    }; // rckid::ui::SharedImage

} // namespace rckid::ui