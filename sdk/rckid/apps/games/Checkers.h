#pragma once

#include "../../ui/form.h"
#include "../../ui/image.h"

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
                uint16_t * bgBuf = buffer;
                for (Coord y = starty; y < starty + numPixels; ++y) {
                    bool is1 = (column / 30 + (starty + y) / 30) % 2 == 0;
                    *bgBuf++ = (is1 ? color1_ : color2_).toRaw();
                }
                Widget::renderColumn(column, buffer, starty, numPixels);
            }

        private:
            ColorRGB color1_ = ColorRGB::Gray();
            ColorRGB color2_ = ColorRGB::DarkGray();
        }; // Checker::Board


        Checkers(): ui::Form<void>{Rect::XYWH(0, 0, 320, 240), /* raw */ true} {
            g_.addChild(board_);
            reset();
        }

    protected:
        void reset() {
            board_.clearChildren();
            for (int i = 0; i < 2; ++i) {
                for (int j = 0; j < 12; ++j) {
                    pieces_[i][j] = new ui::SharedImage{ & pieceBitmaps_[i]};
                    board_.addChild(pieces_[i][j]);
                    pieces_[i][j]->setTransparent(true);
                    pieces_[i][j]->setPos(i * 50 + j * 20, j* 20);
                }
            }
        }

    private:
        Board board_;
        Bitmap pieceBitmaps_[2] = { 
            Icon{assets::icons_24::poo}.toBitmap(), 
            Icon{assets::icons_24::bookmark}.toBitmap()
        };

        ui::SharedImage * pieces_[2][12];


    }; // rckid::Checkers
} // namespace rckid