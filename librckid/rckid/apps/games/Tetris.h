#pragma once

#include "rckid/app.h"
#include "rckid/graphics/framebuffer.h"

namespace rckid {

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

        enum class Tetromino {
            Line, 
            Square,
            T,
            L,
            Lf,
            S,
            Sf
        }; // Tetris::Tetromino

    }; // rckid::Tetris

} // namespace rckid