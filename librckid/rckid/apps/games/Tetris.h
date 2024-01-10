#pragma once

#include "rckid/app.h"
#include "rckid/graphics/framebuffer.h"

namespace rckid {

    /** A simple tetris game
     
        Uses a full-color framebuffer, which is, especially for tetris a bit wasteful in terms of memory. 
     */
    class Tetris : public App<FrameBuffer<ColorRGB>> {
    public:

    protected:

        void onFocus(BaseApp * previous) override {
            App::onFocus(previous);
        }



        void update() override {

        }

        void draw() override {
            //auto & r = renderer();

        }

    private:

        static constexpr uint8_t tetrominos[][4][4] = {
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



        unsigned level_ = 1;

    }; // rckid::Tetris

} // namespace rckid