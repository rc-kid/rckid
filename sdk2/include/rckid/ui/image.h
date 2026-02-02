#pragma once

#include <rckid/graphics/bitmap.h>
#include <rckid/ui/widget.h>


namespace rckid::ui {

    class Image : public Widget {
    public:

        void setRect(Rect rect) override {
            Widget::setRect(rect);
            adjustBitmapPosition();
        }

        Bitmap const & bitmap() const { return bitmap_; }

        void setBitmap(Bitmap value) {
            bitmap_ = std::move(value);
            adjustBitmapPosition();
        }

        HAlign bitmapHAlign() const { return bitmapHAlign_; }

        void setBitmapHAlign(HAlign value) {
            bitmapHAlign_ = value;
            adjustBitmapPosition();
        }

        VAlign bitmapVAlign() const { return bitmapVAlign_; }

        void setBitmapVAlign(VAlign value) {
            bitmapVAlign_ = value;
            adjustBitmapPosition();
        }

        bool bitmapRepeat() const { return bitmapRepeat_; }

        void setBitmapRepeat(bool value) {
            bitmapRepeat_ = value;
        }

        Point bitmapOffset() const { return bitmapOffset_; }

        void setBitmapOffset(Point value) {
            bitmapOffset_ = value;
            adjustBitmapPosition();
        }

        void renderColumn(Coord column, Coord starty, Color::RGB565 * buffer, Coord numPixels) override {
            Widget::renderColumn(column, starty, buffer, numPixels);
            if (bitmapRepeat_) {
                UNIMPLEMENTED;
            } else {
                // adjust the column & starty based on the bitmap position
                column -= bitmapOffset_.x;
                starty -= bitmapOffset_.y;
                //
                if (starty < 0) {
                    buffer += -starty;
                    numPixels += starty;
                    starty = 0;
                }
                // check if we are outside of the bitmap
                if (column < 0 || column >= bitmap_.width() || starty >= bitmap_.height())
                    return;
                // adjust numPixels if we exceed bitmap height
                if (starty + numPixels > bitmap_.height())
                    numPixels = bitmap_.height() - starty;
                // render the bitmap column
                bitmap_.renderColumn(column, starty, numPixels, buffer);
            }
        }

    private:

        void adjustBitmapPosition() {
            Coord x = bitmapOffset_.x;
            Coord y = bitmapOffset_.y;
            switch (bitmapHAlign_) {
                case HAlign::Left:
                    x = 0;
                    break;
                case HAlign::Center:
                    x = (width() - bitmap_.width()) / 2;
                    break;
                case HAlign::Right:
                    x = width() - bitmap_.width();
                    break;
                case HAlign::Manual: // do not change
                    break;
                default:
                    UNREACHABLE;
            }
            switch (bitmapVAlign_) {
                case VAlign::Top:
                    y = 0;
                    break;
                case VAlign::Center:
                    y = (height() - bitmap_.height()) / 2;
                    break;
                case VAlign::Bottom:
                    y = height() - bitmap_.height();
                    break;
                case VAlign::Manual: // do not change
                    break;
                default:
                    UNREACHABLE;
            }
            bitmapOffset_ = Point{x, y};
        }

        Bitmap bitmap_;
        HAlign bitmapHAlign_ = HAlign::Center;
        VAlign bitmapVAlign_ = VAlign::Center;
        Point bitmapOffset_ = Point{0,0};
        bool bitmapRepeat_ = false;

    }; // ui::Image

    struct SetBitmap {
        // this cannot be function as we need move-only functions which only exist in C++20 and higher
        SetBitmap(Bitmap bitmap): bitmap_{std::move(bitmap)} {}
        void operator () (Image * img) const {
            img->setBitmap(std::move(bitmap_));
        }
    private:
        mutable Bitmap bitmap_;
    }; 

    inline auto SetBitmapHAlign(HAlign align) {
        return [align](Image * img) {
            img->setBitmapHAlign(align);
        };
    }

    inline auto SetBitmapVAlign(VAlign align) {
        return [align](Image * img) {
            img->setBitmapVAlign(align);
        };
    }

    inline auto SetBitmapRepeat(bool repeat) {
        return [repeat](Image * img) {
            img->setBitmapRepeat(repeat);
        };
    }

    inline auto SetBitmapOffset(Point offset) {
        return [offset](Image * img) {
            img->setBitmapOffset(offset);
        };
    }

} // namespace rckid::ui