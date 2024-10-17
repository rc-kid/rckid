#pragma once

#include <platform/buffer.h>
#include <rckid/app.h>
#include <rckid/graphics/canvas.h>
#include <rckid/audio/tone.h>

namespace rckid {

    template<typename T>
    class Benchmark : public GraphicsApp<Canvas<ColorRGB>> {
    public:

        static void run() {
            Benchmark app{};
            app.loop();
        }

    protected:
        Benchmark():
            GraphicsApp{Canvas<Color>{320, 240}} {
        }

        void update() override {
            GraphicsApp::update();
            // button A resets the benchmark
            if (btnPressed(Btn::A))
                done_ = false;
        }

        void draw() override {
            if (!done_) {
                g_.fill();
                T core;
                core.run(g_);
                done_ = true;
            }
        }

    private:

        bool done_ = false;

    }; // rckid::Benchmark

} // namespace rckid