#pragma once

#include "rckid/app.h"
#include "rckid/graphics/framebuffer.h"

namespace rckid {

    class Tetris : public App<FrameBuffer<display_profile::RGB>> {
    public:

    protected:

        void onFocus() override {
            App::onFocus();
        }


    }; // rckid::Tetris

} // namespace rckid