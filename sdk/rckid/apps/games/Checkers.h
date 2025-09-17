#pragma once

#include "../../ui/form.h"

namespace rckid {
    class Checkers : public ui::Form<void> {
    public:

        String name() const override { return "Checkers"; }

        /** Simple board widget with alternating squares of different colors. Each square is 30x30 pixels in size, so that full 8x8 board fits exactly in the 240 rows available.
         */
        class Board : public ui::Widget {
        public:
            Board(): ui::Widget{Rect::XYWH(40, 0, 240, 240)} {
            }

            ColorRGB color1() const { return color1_; }
            ColorRGB color2() const { return color2_; }

            void setColor1(ColorRGB c) { color1_ = c; }
            void setColor2(ColorRGB c) { color2_ = c; }

        protected:
          
            void renderColumn(Coord column, uint16_t * buffer, Coord starty, Coord numPixels) override {
                for (Coord y = starty; y < starty + numPixels; ++y) {
                    bool is1 = (column / 30 + (starty + y) / 30) % 2 == 0;
                    *buffer++ = (is1 ? color1_ : color2_).toRaw();
                }
            }

        private:
            ColorRGB color1_ = ColorRGB::Gray();
            ColorRGB color2_ = ColorRGB::DarkGray();
        }; // Checker::Board


        Checkers(): ui::Form<void>{Rect::XYWH(0, 0, 320, 240), /* raw */ true} {
            g_.addChild(board_);
        }

    private:
        Board board_;

    }; // rckid::Checkers
} // namespace rckid