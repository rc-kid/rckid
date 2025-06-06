#pragma once

#include <vector>

#include "widget.h"

namespace rckid::ui {
    
    class Container : public Widget {
    public:
        
        Container() = default;

        Container(Rect rect): Widget(rect) {}

        ~Container() override {
            for (auto w : widgets_)
                delete w;
        }

        void add(Widget * w) {
            widgets_.push_back(w);
        }

        void update() override {
            for (auto w : widgets_)
                w->update();
        }
        
    protected:

        /** Container's information simply renders columns of all child elements in the order they are defined in the list of children, i.e. the earier children can be overdrawn with the later ones.
         */
        void renderColumn(Coord column, uint16_t * buffer, Coord starty, Coord numPixels) override {
            for (auto w : widgets_)
                renderChild(w, column, buffer, starty, numPixels);
        }

    private:
        std::vector<Widget *> widgets_;

    }; // rckid::ui::Container

} // namespace rckid::ui