#pragma once

#include "../rckid.h"
#include "../graphics/bitmap.h"

namespace rckid {

    /** Gauge widget.
     
        This can be used as either a progress bar, or a user controlled gauge. 
     */
    class Gauge {
    public:

        int value() const { return value_; }

        void setMin(int min) { min_ = min; }
        void setMax(int max) { max_ = max; }
        void setValue(int value) { value_ = value; }
        void setStep(int step) { step_ = step; }


        virtual bool update() {
            bool changed = false;
            if (btnPressed(Btn::Left) && (value_ > min_)) {
                value_ -= step_;
                if (value_ < min_)
                    value_ = min_;
                changed = true;
            }
            if (btnPressed(Btn::Right) && (value_ < max_)) {
                value_ += step_;
                if (value_ > max_)
                    value_ = max_;
                changed = true;
            }
            return changed;
        }

        virtual void drawOn(Bitmap<ColorRGB> & surface, Rect where) {
            surface.fill(bg_, where);
            surface.fill(gauge_, Rect::XYWH(where.topLeft(), where.w * (value_ - min_) / (max_ - min_), where.h));

        }

    private:

        int min_ = 0;
        int max_ = 100;
        int step_ = 10;
        int value_ = 50;

        ColorRGB bg_ = ColorRGB{0x80,0x80, 0x80};
        ColorRGB gauge_ = ColorRGB{0xff, 0xc0, 0xc0};

    }; 


} // namespace rckid 