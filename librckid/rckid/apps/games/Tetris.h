#pragma once

#include "rckid/app.h"
#include "rckid/graphics/framebuffer.h"

namespace rckid {

    /** A simple tetris game
     
        Uses a full-color framebuffer, which is, especially for tetris a bit wasteful in terms of memory. 
     */
    class Tetris : public FrameBufferApp<ColorRGB> {
    public:

    protected:

        void onFocus() override {
            FrameBufferApp::onFocus();
            reset();
        }



        void update() override {

        }

        void draw() override {

            // draw the area, we only draw the changed pieces
            for (int i = 0, c = 0; c < 10; ++c) {
                for (int r = 0; r < 24; ++r, ++i) {
                    if (tiles_[i] & 0x80) {
                        fb_.fill(getTileRect(c, r, tiles_[i]), getTileColor(tiles_[i]));
                        tiles_[i] &= 0x7f;
                    }
                }
            }
            // draw the current tetromino
        }


        void reset() {
            for (int i = 0; i < TILEMAP_ROWS * TILEMAP_COLS; ++i)
                tiles_[i] = 0x80 + get_rand_32() % 7;

        }

        Rect getTileRect(int col, int row, uint8_t tile) {
            int b = (tile & 0x40) ? 0 : 1;
            return Rect::XYWH(160 - TILEMAP_COLS * TILE_SIZE / 2 + col * TILE_SIZE + b, row * TILE_SIZE + b, TILE_SIZE - 2 * b, TILE_SIZE - 2 * b);
        }

        Color getTileColor(uint8_t tile) {
            return Color::RGB(
                (tile & 1) ? 255 : 0, 
                (tile & 2) ? 255 : 0,
                (tile & 4) ? 255 : 0 
            );
        }

        bool canPlaceTetromino(int x, int y) {
            // TODO
            return false;
        }

        /** Places the given tetromino on the board at given coordinates. Returns true if the placement was successful (i.e. no collision), false otherwise. 
         
         */
        bool placeTetromino(int x, int y, int t, uint8_t tileColor) {
            // TODO
            //uint8_t const ** tData = tetrominos_[t];
            return false;
        }


    private:

        static constexpr int TILE_SIZE = 12;
        static constexpr int TILEMAP_ROWS = 240 / TILE_SIZE;
        static constexpr int TILEMAP_COLS = 10;

        static constexpr uint8_t tetrominos_[][4][4] = {
            { // 0 = -----
                {1, 1, 1, 1},
                {0, 0, 0, 0},
                {0, 0, 0, 0},
                {0, 0, 0, 0},
            },
            { // 1 = .|.
                {0, 1, 0, 0},
                {1, 1, 1, 0},
                {0, 0, 0, 0},
                {0, 0, 0, 0},
            },
            { // 2 = |___
                {1, 0, 0, 0},
                {1, 1, 1, 0},
                {0, 0, 0, 0},
                {0, 0, 0, 0},
            },
            { // 3 = ___|
                {0, 0, 0, 1},
                {0, 1, 1, 1},
                {0, 0, 0, 0},
                {0, 0, 0, 0},
            },
            { // 4 = --__
                {1, 1, 0, 0},
                {0, 1, 1, 0},
                {0, 0, 0, 0},
                {0, 0, 0, 0},
            },
            { // 5 = __--
                {0, 0, 1, 1},
                {0, 1, 1, 0},
                {0, 0, 0, 0},
                {0, 0, 0, 0},
            },
            { // 6 = .
                {1, 1, 0, 0},
                {1, 1, 0, 0},
                {0, 0, 0, 0},
                {0, 0, 0, 0},
            },
        };

        uint8_t tiles_[TILEMAP_ROWS * TILEMAP_COLS];

        unsigned lines_ = 0;
        unsigned score_ = 0;

    }; // rckid::Tetris

} // namespace rckid