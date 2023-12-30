#pragma once

#include "rckid/app.h"
#include "rckid/sd.h"
#include "rckid/graphics/framebuffer.h"

namespace rckid {

    class SDCardTest : public App<FrameBuffer> {
    public:

        SDCardTest() {
            initialize();
        }

    protected:

        void initialize() {
            SD::mount();
            sdSize_ = SD::totalBytes() / 1000000_u64;
        }

        void update() override {
        }

        void draw() {
            Renderer & r = renderer();
            r.text(0,0);
            r.text() << "Total: " << sdSize_ << "\n";
            App::draw();
        }

    private:

        size_t sdSize_; // SD card size in megabytes

    }; 
}