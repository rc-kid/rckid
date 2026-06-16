#pragma once

#include <rckid/ui/widget.h>

namespace rckid::ui {

    /** A simple binary toggle (on/off)
     */
    class Toggle : public Widget {
    public:

        bool value() const { return value_; }

        void setValue(bool value) {
            if (value_ == value)
                return;
            value_ = value;
        }

    private:
        bool value_;

    }; // ui::Toggle

} // namespace rckid::ui