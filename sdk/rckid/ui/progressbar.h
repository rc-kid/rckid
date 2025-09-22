#pragma once

#include "../graphics/color.h"
#include "widget.h"
#include "style.h"

namespace rckid::ui {
    class ProgressBar : public Widget {
    public:
        ProgressBar(Rect rect, int32_t min = 0, int32_t max = 100, int32_t value = 50):
            Widget{rect},
            min_{min},
            max_{max},
            value_{value},
            threshold_{0} {
            recalculateThreshold();
        }

        void reset(Rect rect, int32_t min = 0, int32_t max = 100, int32_t value = 50) {
            min_ = min;
            max_ = max;
            value_ = value;
            setRect(rect);
            recalculateThreshold();
        }

        ColorRGB fg() const { return fg_; }
        ColorRGB bg() const { return bg_; }
        int32_t min() const { return min_; }
        int32_t max() const { return max_; }
        int32_t value() const { return value_; }

        void setFg(ColorRGB fg) { fg_ = fg; }
        
        void setBg(ColorRGB bg) { bg_ = bg; }
        
        void setMin(int32_t min) { 
            min_ = min; 
            recalculateThreshold();
        }

        void setMax(int32_t max) { 
            max_ = max; 
            recalculateThreshold();
        }

        void setRange(int32_t min, int32_t max) { 
            min_ = min; 
            max_ = max; 
            recalculateThreshold();
        }

        void setValue(int32_t value) { 
            if (value < min_)
                value = min_;
            if (value > max_)
                value = max_;
            value_ = value; 
            recalculateThreshold();
        }

    protected:
        void renderColumn(Coord column, uint16_t * buffer, [[maybe_unused]] Coord starty, Coord numPixels) override {
            // determine color based on the column


            uint16_t c = (column < threshold_) ? fg_.toRaw() : bg_.toRaw();;
            // now draw the column with the given color
            // just fill with the color
            while (numPixels-- > 0) {
                *buffer = c;
                ++buffer;
            }
        }

        void recalculateThreshold() {
            if (value_ <= min_)
                threshold_ = 0;
            else if (value_ >= max_)
                threshold_ = width();
            else
                threshold_ = ((value_ - min_) * width()) / (max_ - min_);
        }

        void resize() override {
            Widget::resize();
            recalculateThreshold();
        }

    private:
        ColorRGB fg_{Style::accentFg()};
        ColorRGB bg_{Style::accentBg()};
        int32_t min_;
        int32_t max_;
        int32_t value_;
        Coord threshold_;

    }; // ui::ProgressBar

} // namespace rckid::ui