#pragma once

#include <vector>

#include "../memory.h"
#include "../graphics/color.h"
#include "../graphics/geometry.h"

namespace rckid::ui {

    class Widget {
    public:

        Widget(Rect rect): rect_{rect} {}

        virtual ~Widget() = default;

        Rect rect() const { return rect_; }

        bool visible() const { return visible_; }


        template<typename T>
        T * addChild(T * child) {
            children_.push_back(unique_ptr<Widget>(child));
            return child;
        }

        /** Renders vertical column of the the widget to given color buffer. 
         */
        virtual void renderColumn(Coord column, Color::RGB565 * buffer, Coord starty, Coord numPixels) {

        }

    protected:

    private:

        template<typename T>
        friend class App;

        Rect rect_;
        bool visible_ = true;

        std::vector<unique_ptr<Widget>> children_;

        void initializeDisplay() {
            hal::display::enable(rect_, hal::display::RefreshDirection::ColumnFirst);

        }

        void render();


    }; // ui::Widget


} // namespace rckid