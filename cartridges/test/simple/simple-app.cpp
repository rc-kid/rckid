#include "rckid/app.h"
#include "rckid/stats.h"
#include "rckid/graphics/framebuffer.h"

using namespace rckid;

class SimpleApp : public App<FrameBuffer<Color256>> {
public:

protected:

    void update() override {

    }

    void draw() override {
        driver_.fill();
        driver_.textMultiline(0, 0) 
            << "SimpleApp:      " << t << "\n"
            << "FPS:            " << stats::fps() << "\n"
            << "waitTick:       " << stats::waitTickUs() << "\n"
            << "updateUs:       " << stats::updateUs() << "\n"
            << "tickUs:         " << stats::tickUs() << "\n"
            << "waitRender:     " << stats::waitRenderUs() << "\n"
            << "drawUs:         " << stats::drawUs() << "\n"
            << "waitVSync:      " << stats::waitVSyncUs() << "\n"
            << "render:         " << stats::renderUs() << "\n"
            << "display update: " << stats::displayUpdateUs();
        ++t;
    }


    unsigned t = 0;

}; // SimpleApp

int main() {
    rckid::initialize();
    SimpleApp{}.run();
}