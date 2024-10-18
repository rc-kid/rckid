#pragma once

#include <platform/buffer.h>
#include <rckid/app.h>
#include <rckid/graphics/canvas.h>
#include <rckid/audio/tone.h>

namespace rckid {

    /** A simple benchmark template. 
     
        The app is parametrixed by a benchmark core which is a simple class with default constructor and a run() method that takes the canvas as input and runs the benchmark, printing the results on the given canvas. 

        Pressing A restarts the benchmark from beginning, while pressing B returns (as any other app should). 

        TODO this is very basic and perhaps the actual canvas (graphics) should be templated and specified by the benchmark core itself.  
     */
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
            g_.fill(Rect::WH(320, 20));
            Header::drawOn(g_);
        }

    private:

        bool done_ = false;

    }; // rckid::Benchmark

} // namespace rckid