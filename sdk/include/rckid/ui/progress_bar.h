#pragma once

#include <rckid/ui/widget.h>

namespace rckid::ui {

    /** A very simple progress bar.
     */
    class ProgressBar : public Widget {
    public:
        Color fg() const { return fg_; }
        Color bg() const { return bg_; };
        int32_t min() const { return min_; }
        int32_t max() const { return max_; }
        int32_t value() const { return value_; };

        void setFg(Color color) { fg_ = color; }
        void setBg(Color color) { bg_ = color; }
        void setMin(int32_t min) { min_ = min; }
        void setMax(int32_t max) { max_ = max; }
        void setValue(int32_t value) { value_ = value; }

        bool changeValueBy(int32_t by) {
            if (by > 0) {
                if (value_ == max_)
                    return false;
                if (by > max_ - value_)
                    by = max_ - value_;
            } else {
                if (value_ == min_)
                    return false;
                if (by < min_ - value_)
                    by = min_ - value_;
            }
            value_ += by;
            return true;
        }

        bool inc(int32_t by = 1) { return changeValueBy(by); }
        bool dec(int32_t by = 1) { return changeValueBy(-by); }

        void renderColumn(Coord column, Coord startRow, Color::RGB565 * buffer, Coord numPixels) override {
            // determine the column at which we are switching from the barto background color
            Color::RGB565 c{bg_.toRGB565()};
            if (max_ > min_) {
                Coord threshold = width() * (value_ - min_) / (max_ - min_);
                if (column < threshold)
                    c = Color::RGB565{fg_.toRGB565()};
            }
            // draw the progress bar
            memset16(reinterpret_cast<uint16_t*>(buffer), c, numPixels);
            // and any chlildren
            Widget::renderColumn(column, startRow, buffer, numPixels);
        }

        void applyStyle(Style const & style) override {
            Widget::applyStyle(style);
            fg_ = style.accentFg();
            bg_ = style.accentBg();
        }

    protected:

    private:

        Color fg_;
        Color bg_;
        int32_t min_;
        int32_t max_;
        int32_t value_;

    }; // ProgressBar

    struct SetMin {
        int32_t min;
        SetMin(int32_t min): min{min} {}
    };
    template<typename T>
    inline with<T> operator << (with<T> w, SetMin sm) {
        w->setMin(sm.min);
        return w;
    }

    struct SetMax {
        int32_t max;
        SetMax(int32_t max): max{max} {}
    };
    template<typename T>
    inline with<T> operator << (with<T> w, SetMax sm) {
        w->setMax(sm.max);
        return w;
    }

    struct SetRange {
        int32_t min;
        int32_t max;
        SetRange(int32_t min, int32_t max): min{min}, max{max} {}
    };

    template<typename T>
    inline with<T> operator << (with<T> w, SetRange sr) {
        w->setMin(sr.min);
        w->setMax(sr.max);
        return w;
    }

    struct SetValue {
        int32_t value;
        SetValue(int32_t value): value{value} {}
    };
    template<typename T>
    inline with<T> operator << (with<T> w, SetValue sv) {
        w->setValue(sv.value);
        return w;
    }
    
}