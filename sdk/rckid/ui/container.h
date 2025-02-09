#pragma once

#include <vector>

#include "widget.h"

namespace rckid::ui {
    
    class Container : public Widget {
    public:
        
        Container() = default;

        ~Container() override {
            for (auto w : widgets_)
                Heap::tryFree(w);
        }

        void add(Widget * w) {
            widgets_.push_back(w);
        }

        /** Container's information simply renders columns of all child elements in the order they are defined in the list of children, i.e. the earier children can be overdrawn with the later ones.
         */
        void renderColumn(Coord column, Pixel * buffer, Coord starty, Coord numPixels) override {
            for (auto w : widgets_)
                renderChild(w, column, buffer, starty, numPixels);
        }

    protected:
        std::vector<Widget *> widgets_;

    }; // rckid::ui::Container

} // namespace rckid::ui