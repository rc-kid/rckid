#pragma once

#include <rckid/ui/widget.h>

namespace rckid::ui {

    /** Chevron-shaped container. 
     
        The chevron-shaped containers are predominantly used in the game engine user interface.
     */
    class Chevron : public Widget {
    public:
        enum class Edge {
            Straight,
            In,
            Out,
        };

        Color bg() const { return bg_; }
        void setBg(Color value) { bg_ = value; }

        Edge leftEdge() const { return leftEdge_; }
        Edge rightEdge() const { return rightEdge_; }

        void setLeftEdge(Edge value) { leftEdge_ = value; }
        void setRightEdge(Edge value) { rightEdge_ = value; }

        void renderColumn(Coord column, Coord startRow, Color::RGB565 * buffer, Coord numPixels) override {
            Color::RGB565 c{bg_};

            if (column <= chevronWidth(leftEdge_)) {
                renderChevronColumn(leftEdge_, column, c, startRow, buffer, numPixels);
            } else if (column >= width() - chevronWidth(rightEdge_)) {
                renderChevronColumn(rightEdge_, column - (width() - chevronWidth(rightEdge_)), c, startRow, buffer, numPixels);
            } else {
                memset16(reinterpret_cast<uint16_t*>(buffer), c, std::min(width() - startRow, numPixels));
            }

            Widget::renderColumn(column, startRow, buffer, numPixels);
        }

    private:

        Coord chevronWidth(Edge edge) {
            if (edge == Edge::Straight)
                return 0;
            else 
                return height() / 2;
        }

        void renderChevronColumn(Edge edge, Coord column, Color::RGB565 color, Coord startRow, Color::RGB565 * buffer, Coord numPixels) {
            switch (edge) {
                case Edge::In: // >|
                    for (Coord y = 0; y < column; ++y)
                        renderPixelInBuffer(y, color, startRow, buffer, numPixels);
                    for (Coord y = height() - column, ye = height(); y < ye; ++y)
                        renderPixelInBuffer(y, color, startRow, buffer, numPixels);
                    break;
                case Edge::Out: // |>
                    for (Coord y = column, ye = height() - column; y < ye; ++y)
                        renderPixelInBuffer(y, color, startRow, buffer, numPixels);
                    break;
                default:
                    memset16(reinterpret_cast<uint16_t*>(buffer), color, std::min(width() - startRow, numPixels));
                    break;
            }
        }

        Color bg_;

        Edge leftEdge_;
        Edge rightEdge_;
    }; // rckid::ui::Chevron


    struct SetLeftEdge {
        Chevron::Edge value;
        SetLeftEdge(Chevron::Edge value): value{value} {}
    };
    template<typename T>
    inline with<T> operator << (with<T> w, SetLeftEdge le) {
        w->setLeftEdge(le.value);
        return w;
    }
    
    struct SetRightEdge {
        Chevron::Edge value;
        SetRightEdge(Chevron::Edge value): value{value} {}
    };
    template<typename T>
    inline with<T> operator << (with<T> w, SetRightEdge le) {
        w->setRightEdge(le.value);
        return w;
    }


} // namespace rckid::ui