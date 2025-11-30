#pragma once

#include "../../ui/form.h"
#include "../../ui/image.h"

namespace rckid {
    class Checkers : public ui::Form<void> {
    public:

        String name() const override { return "Checkers"; }

        /** Games are buddgeted.
         */
        bool isBudgeted() const override { return true; }

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

            void reset() {
                clearChildren();
                for (int i = 0; i < 2; ++i) {
                    for (int j = 0; j < 12; ++j) {
                        pieces_[i][j] = new ui::SharedImage{ & pieceBitmaps_[i]};
                        addChild(pieces_[i][j]);
                        pieces_[i][j]->setTransparent(true);
                        pieces_[i][j]->setPos(i * 50 + j * 20, j* 20);
                    }
                }
                uint8_t p1 = 0;
                uint8_t p2 = 0;
                for (int y = 0; y < 8; ++y) {
                    for (int x = 0; x < 8; ++x) {
                        bool is1 = (x + y) % 2 == 0;
                        if (y < 3 && !is1) // player 1 pieces
                            setBoard(x, y, PLAYER1 | (p1++));
                        else if (y > 4 && !is1) // player 2 pieces
                            setBoard(x, y, PLAYER2 | (p2++));
                        else
                            setBoard(x, y, 0); // empty
                    }
                }
            }


        protected:
          
            void renderColumn(Coord column, uint16_t * buffer, Coord starty, Coord numPixels) override {
                uint16_t * bgBuf = buffer;
                for (Coord y = starty; y < starty + numPixels; ++y) {
                    bool is1 = (column / 30 + (starty + y) / 30) % 2 == 0;
                    *bgBuf++ = (is1 ? color1_ : color2_).toRaw();
                }
                Widget::renderColumn(column, buffer, starty, numPixels);
            }

            void setBoard(uint8_t x, uint8_t y, uint8_t piece) {
                board_[x][y] = piece;
                uint8_t player = (piece & 0xf0) >> 4;
                if (player != 1 && player != 2)
                    return;
                piece = piece & 0xf;
                ASSERT(piece < 12);
                pieces_[player - 1][piece]->setPos(x * 30 + 3, y * 30 + 3);
            }


        private:
            ColorRGB color1_ = ColorRGB::Gray();
            ColorRGB color2_ = ColorRGB::DarkGray();


            Bitmap pieceBitmaps_[2] = { 
                Icon{assets::icons_24::poo}.toBitmap(), 
                Icon{assets::icons_24::bookmark}.toBitmap()
            };

            ui::SharedImage * pieces_[2][12];

            static constexpr uint8_t EMPTY = 0x0;
            static constexpr uint8_t PLAYER1 = 0x10;
            static constexpr uint8_t PLAYER2 = 0x20;

            uint8_t board_[8][8] = {0x0};

        }; // Checker::Board


        Checkers(): ui::Form<void>{Rect::XYWH(0, 0, 320, 240), /* raw */ true} {
            board_ = g_.addChild(new Board{});
            board_->reset();
        }

    private:
        Board * board_;


    }; // rckid::Checkers
} // namespace rckid